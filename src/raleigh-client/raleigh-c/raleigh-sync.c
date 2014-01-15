/*
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include <unistd.h>

#include <zcl/semaphore.h>
#include <zcl/buffer.h>
#include <zcl/iopoll.h>
#include <zcl/iovec.h>
#include <zcl/fd.h>

#include "generated/rpc.h"
#include "raleigh.h"
#include "private.h"

/* ============================================================================
 *  PRIVATE Protocol helpers
 */
#if !__RALEIGH_USE_ASYNC_AND_LATCH
struct rpc_head {
  uint64_t req_id;
  uint64_t msg_type;
  uint32_t msg_size;
};

static int __parse_head (const unsigned char *buffer, size_t bufsize, struct rpc_head *head) {
  uint8_t head_len;
  uint8_t version;
  uint8_t len[2];
  uint32_t magic;

  if (Z_UNLIKELY(bufsize < 8)) {
    fprintf(stderr, "Expected at least 8 bytes for the rpc-header, got %zu\n", bufsize);
    return(-1);
  }

  version        = buffer[0];
  head->msg_size = buffer[1] | buffer[2] << 8 | buffer[3] << 16;
  magic          = buffer[4] | buffer[5] << 8 | buffer[6] << 16 | buffer[7] << 24;

  if (Z_UNLIKELY(version != 0)) {
    fprintf(stderr, "Invalid Version %"PRIu8, version);
    return(-2);
  }

  if (Z_UNLIKELY(magic != 0xaacc33d5)) {
    fprintf(stderr, "Invalid Magic %"PRIx32"\n", magic);
    return(-3);
  }

  len[0] = 1 + z_fetch_3bit(buffer[8], 5);
  len[1] = 1 + z_fetch_3bit(buffer[8], 2);
  buffer += 9;

  z_decode_uint64(buffer, len[0], &(head->msg_type)); buffer += len[0];
  z_decode_uint64(buffer, len[1], &(head->req_id));
  head_len = 9 + len[0] + len[1];

  if (Z_UNLIKELY(bufsize != (8 + head->msg_size))) {
    fprintf(stderr, "Response length doesn't match %zu - head_len=%"PRIu8" msg_len=%"PRIu32"\n",
                    bufsize, head_len, head->msg_size);
    return(-4);
  }

  return(head_len);
}

static int __parse_and_verify_head (const unsigned char *buffer, size_t bufsize, struct rpc_head *head, uint64_t req_id, uint64_t msg_type) {
  int head_len;

  if ((head_len = __parse_head(buffer, bufsize, head)) < 0)
    return(head_len);

  if (Z_UNLIKELY(req_id != head->req_id)) {
    fprintf(stderr, "Response Id doesn't match: expected %"PRIu64" got %"PRIu64"\n", req_id, head->req_id);
    return(-2);
  }

  if (Z_UNLIKELY(msg_type != head->msg_type)) {
    fprintf(stderr, "Response type doesn't match: expected %"PRIu64" got %"PRIu64"\n", msg_type, head->msg_type);
    return(-3);
  }

  return(head_len);
}

static ssize_t __send_message(int fd, uint64_t msg_type, uint64_t req_id, void *buf, size_t bufsize) {
  unsigned char rpc_head[32];
  unsigned char head[8];
  struct iovec iov[3];
  uint8_t len[2];
  size_t total_size;

  len[0] = z_uint64_size(msg_type);
  len[1] = z_uint64_size(req_id);
  rpc_head[0] = ((len[0] - 1) << 5) | ((len[1] - 1) << 2) | (1 << 1);
  z_encode_uint(rpc_head + 1, len[0], msg_type);
  z_encode_uint(rpc_head + 1 + len[0], len[1], req_id);

  total_size = 1 + len[0] + len[1] + bufsize;
  head[0] = 0;
  head[1] = total_size & 0xff;
  head[2] = (total_size >>  8) & 0xff;
  head[3] = (total_size >> 16) & 0xff;
  head[4] = 0xd5;
  head[5] = 0x33;
  head[6] = 0xcc;
  head[7] = 0xaa;

  iov[0].iov_base = head;
  iov[0].iov_len  = 8;
  iov[1].iov_base = rpc_head;
  iov[1].iov_len  = 1 + len[0] + len[1];
  iov[2].iov_base = buf;
  iov[2].iov_len  = bufsize;

  return(z_fd_writev(fd, iov, 3));
}
#endif

/* ============================================================================
 *  Sync Ping
 */
#if __RALEIGH_USE_ASYNC_AND_LATCH
static void __ping_callback (void *udata) {
  z_latch_release(Z_LATCH(udata));
}
#endif

int raleigh_ping (raleigh_client_t *self) {
#if __RALEIGH_USE_ASYNC_AND_LATCH
  z_latch_t latch;
  int r = 0;
  z_latch_open(&latch);
  r = raleigh_ping_async(self, __ping_callback, &latch);
  z_latch_wait(&latch, 1);
  z_latch_close(&latch);
  return(r);
#else
  struct server_ping_response resp;
  struct server_ping_request req;
  unsigned char buffer[128];
  struct rpc_head rpc_head;
  z_iovec_reader_t reader;
  z_buffer_t reqbuf;
  struct iovec iov[1];
  uint64_t req_id = 0;
  ssize_t rd;
  int head;

  server_ping_request_alloc(&req);
  z_buffer_alloc(&reqbuf);
  server_ping_request_write(&req, &reqbuf);
  __send_message(Z_IOPOLL_ENTITY_FD(self), 90, 0, reqbuf.block, reqbuf.size);
  z_buffer_free(&reqbuf);
  server_ping_request_free(&req);

  /* TODO: loop */
  rd = z_fd_read(Z_IOPOLL_ENTITY_FD(self), buffer, sizeof(buffer));
  if (Z_UNLIKELY(rd < 0)) {
    perror("read()");
    return(1);
  }

  head = __parse_and_verify_head(buffer, rd, &rpc_head, req_id, 90);
  if (Z_UNLIKELY(head < 0)) {
    return(1);
  }

  iov[0].iov_base = buffer + head;
  iov[0].iov_len = rpc_head.msg_size;
  z_iovec_reader_open(&reader, iov, 1);
  server_ping_response_alloc(&resp);
  server_ping_response_parse(&resp, &reader, rpc_head.msg_size);
  server_ping_response_free(&resp);
  return(0);
#endif
}
