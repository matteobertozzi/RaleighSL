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

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <zcl/threading.h>
#include <zcl/coding.h>
#include <zcl/socket.h>
#include <zcl/debug.h>
#include <zcl/time.h>


#define NREQUESTS    ((uint64_t)200000)
#define NTHREADS     1
#define USEFORK      0

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
  return(writev(fd, iov, 3));
}

static int __parse_head (const unsigned char *buffer, size_t bufsize,
                         uint64_t *msg_type, uint64_t *req_id, uint32_t *msg_size)
{
  uint8_t head_len;
  uint8_t version;
  uint8_t len[2];
  uint32_t magic;

  version   = buffer[0];
  *msg_size = buffer[1] | buffer[2] << 8 | buffer[3] << 16;
  magic     = buffer[4] | buffer[5] << 8 | buffer[6] << 16 | buffer[7] << 24;

  if (Z_UNLIKELY(version != 0)) {
    fprintf(stderr, "Invalid Version %"PRIu8, version);
    return(-1);
  }

  if (Z_UNLIKELY(magic != 0xaacc33d5)) {
    fprintf(stderr, "Invalid Magic %"PRIx32"\n", magic);
    return(-2);
  }

  len[0] = 1 + z_fetch_3bit(buffer[8], 5);
  len[1] = 1 + z_fetch_3bit(buffer[8], 2);
  buffer += 9;

  z_decode_uint64(buffer, len[0], msg_type); buffer += len[0];
  z_decode_uint64(buffer, len[1], req_id);
  head_len = 9 + len[0] + len[1];

  if (Z_UNLIKELY(bufsize != (8 + *msg_size))) {
    fprintf(stderr, "Response length doesn't match %zu - head_len=%"PRIu8" msg_len=%"PRIu32"\n",
                    bufsize, head_len, *msg_size);
    return(-3);
  }

  return(head_len);
}

static void __read_and_verify (int sock, uint64_t req_id, uint64_t msg_type) {
  uint64_t resp_id, resp_type;
  unsigned char buffer[256];
  uint32_t resp_size;
  ssize_t rd;
  int head;

  rd = read(sock, buffer, sizeof(buffer));
  if (Z_UNLIKELY(rd < 0)) perror("read()");

  head = __parse_head(buffer, rd, &resp_type, &resp_id, &resp_size);
  if (Z_UNLIKELY(head < 0)) {
    exit(1);
  }

  if (Z_UNLIKELY(req_id != resp_id)) {
    fprintf(stderr, "Response Id doesn't match: resp_id=%"PRIu64" req_id=%"PRIu64"\n", resp_id, req_id);
    exit(1);
  }

  if (Z_UNLIKELY(resp_type != msg_type)) {
    fprintf(stderr, "Response type doesn't match: %"PRIu64"\n", resp_type);
    exit(1);
  }
}

static void __create_number (int sock, uint64_t req_id) {
  unsigned char buffer[128];
  size_t bufsize;
  ssize_t rd;

  bufsize = z_encode_field(buffer, 1, 1);
  buffer[bufsize++] = 'X';

  bufsize += z_encode_field(buffer + bufsize, 2, 6);
  z_memcpy(buffer + bufsize, "number", 6);
  bufsize += 7;

  rd = __send_message(sock, 11, req_id, buffer, bufsize);
  if (Z_UNLIKELY(rd < 0)) perror("write()");

  __read_and_verify(sock, req_id, 11);
}

static void __number_get (int sock, uint64_t req_id, uint64_t oid) {
  unsigned char buffer[128];
  unsigned int oid_len;
  size_t bufsize;
  ssize_t rd;

  oid_len = z_uint64_size(oid);
  bufsize = z_encode_field(buffer, 1, oid_len);
  z_encode_uint64(buffer + bufsize, oid_len, oid);
  bufsize += oid_len;

  rd = __send_message(sock, 30, req_id, buffer, bufsize);
  if (Z_UNLIKELY(rd < 0)) perror("write()");

  __read_and_verify(sock, req_id, 30);
}

static void __server_ping (int sock, uint64_t req_id) {
  ssize_t rd;

  rd = __send_message(sock, 90, req_id, NULL, 0);
  if (Z_UNLIKELY(rd < 0)) perror("write()");

  __read_and_verify(sock, req_id, 90);
}

void *__execute_test (void *args) {
  uint64_t req_id = 0;
  uint64_t st, et;
  int sock;

  sock = z_socket_tcp_connect("localhost", "11215", NULL);
  if (sock < 0) {
    perror("sock()");
    return(NULL);
  }

  fprintf(stderr, "Start Test!\n");

  /* create new number */
  __create_number(sock, req_id++);

  /* number get */
  req_id = 0;
  st = z_time_nanos();
  while (req_id < NREQUESTS) {
    __number_get(sock, req_id++, 1);
  }
  et = z_time_nanos();
  printf("number_get reqs %"PRIu64" %.3fsec (%.2freq/sec)\n", req_id, (et - st) / 1000000000.0f,
         req_id / ((et - st) / 1000000000.0f));

  /* ping */
  req_id = 0;
  st = z_time_nanos();
  while (req_id < NREQUESTS) {
    __server_ping(sock, req_id++);
  }
  et = z_time_nanos();
  printf("server_ping reqs %"PRIu64" %.3fsec (%.2freq/sec)\n", req_id, (et - st) / 1000000000.0f,
         req_id / ((et - st) / 1000000000.0f));

#if 0
  /* Keep the connection open for a while */
  fprintf(stderr, "Wait before closing...\n");
  usleep(5000000);
#endif

  close(sock);
  return(NULL);
}

int main (int argc, char **argv) {
  uint64_t et, st = z_time_nanos();
  uint64_t nrequests;
#if !USEFORK
  z_thread_t thread[NTHREADS];
  int i;

  for (i = 0; i < NTHREADS; ++i) {
    z_thread_start(&(thread[i]), __execute_test, NULL);
  }

  for (i = 0; i < NTHREADS; ++i) {
    z_thread_join(&(thread[i]));
  }
#else
  pid_t pid[NTHREADS];
  int i;

  for (i = 0; i < NTHREADS; ++i) {
    pid[i] = fork();
    if (pid[i] == 0) {
      __execute_test(NULL);
      return(0);
    }
  }

  for (i = 0; i < NTHREADS; ++i) {
    int status;
    waitpid(pid[i], &status, 0);
  }
#endif
  et = z_time_nanos();
  nrequests = (1 + (NREQUESTS * 2) * NTHREADS);
  //nrequests = ((NREQUESTS * 1) * NTHREADS);
  printf("%d clients estimate avg-reqs %"PRIu64" %.3fsec (%.2freq/sec)\n", NTHREADS,
         nrequests, (et - st) / 1000000000.0f,
         nrequests / ((et - st) / 1000000000.0f));
  return(0);
}