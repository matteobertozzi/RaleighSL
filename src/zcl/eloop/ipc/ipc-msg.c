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

#include <zcl/int-coding.h>
#include <zcl/ipc-msg.h>
#include <zcl/string.h>
#include <zcl/time.h>

/*
 * +------------------+
 * | ---- |  11 |  11 | pkg-type, msg-type, fwd
 * |  111 |  11 | 111 | msg-id, body, blob
 * +------------------+
 * |     msg type     | (min 1, max 4) 0b11
 * |     msg id       | (min 1, max 8) 0b111
 * +------------------+
 * |    fwd length    | (min 0, max 3) 0b11
 * +------------------+
 * |   body length    | (min 0, max 3) 0b11
 * +------------------+
 * |   data length    | (min 0, max 4) 0b111
 * +------------------+
 * /     fwd blob     /
 * +------------------+
 * /    body blob     /
 * +------------------+
 * /    data blob     /
 * +------------------+
 */

/* ============================================================================
 *  ipc-msg stats helpers
 */
#define __STATS_TIME_HISTO_NBOUNDS   z_fix_array_size(__STATS_TIME_HISTO_BOUNDS)
#define __STATS_SIZE_HISTO_NBOUNDS   z_fix_array_size(__STATS_SIZE_HISTO_BOUNDS)

static const uint64_t __STATS_TIME_HISTO_BOUNDS[22] = {
  Z_TIME_MICROS_TO_NANOS(5),   Z_TIME_MICROS_TO_NANOS(10),  Z_TIME_MICROS_TO_NANOS(25),
  Z_TIME_MICROS_TO_NANOS(60),  Z_TIME_MICROS_TO_NANOS(80),  Z_TIME_MICROS_TO_NANOS(125),
  Z_TIME_MICROS_TO_NANOS(200), Z_TIME_MICROS_TO_NANOS(350), Z_TIME_MICROS_TO_NANOS(500),
  Z_TIME_MICROS_TO_NANOS(750), Z_TIME_MILLIS_TO_NANOS(5),   Z_TIME_MILLIS_TO_NANOS(10),
  Z_TIME_MILLIS_TO_NANOS(25),  Z_TIME_MILLIS_TO_NANOS(50),  Z_TIME_MILLIS_TO_NANOS(75),
  Z_TIME_MILLIS_TO_NANOS(100), Z_TIME_MILLIS_TO_NANOS(250), Z_TIME_MILLIS_TO_NANOS(350),
  Z_TIME_MILLIS_TO_NANOS(500), Z_TIME_MILLIS_TO_NANOS(750), Z_TIME_SECONDS_TO_NANOS(1),
  0xffffffffffffffffull,
};

static const uint64_t __STATS_SIZE_HISTO_BOUNDS[26] = {
    16,  32,  48,  64,  80,  96, 112, 128, 256, 512,
  Z_KIB(1),   Z_KIB(2),   Z_KIB(4),   Z_KIB(8),   Z_KIB(16),
  Z_KIB(32),  Z_KIB(64),  Z_KIB(128), Z_KIB(256), Z_KIB(512),
  Z_MIB(1),   Z_MIB(8),   Z_MIB(16),  Z_MIB(32),  Z_MIB(64),
  0xffffffffffffffffll,
};

void z_ipc_msg_stats_init (z_ipc_msg_stats_t *self) {
  z_histogram_init(&(self->latency),  __STATS_TIME_HISTO_BOUNDS,
                   self->latency_events,  __STATS_TIME_HISTO_NBOUNDS);
  z_histogram_init(&(self->fwd_size),  __STATS_SIZE_HISTO_BOUNDS,
                   self->fwd_size_events,  __STATS_SIZE_HISTO_NBOUNDS);
  z_histogram_init(&(self->body_size),  __STATS_SIZE_HISTO_BOUNDS,
                   self->body_size_events,  __STATS_SIZE_HISTO_NBOUNDS);
  z_histogram_init(&(self->data_size),  __STATS_SIZE_HISTO_BOUNDS,
                   self->data_size_events,  __STATS_SIZE_HISTO_NBOUNDS);
}

void z_ipc_msg_stats_dump (z_ipc_msg_stats_t *self,
                           const char *msg_type,
                           FILE *stream)
{
  fprintf(stream, "IPC-MSG %s Stats\n", msg_type);
  fprintf(stream, "==================================\n");

  fprintf(stream, "%s Latency", msg_type);
  z_histogram_dump(&(self->latency), stream, z_human_time_d);
  fprintf(stream, "\n");

  fprintf(stream, "%s Fwd Size", msg_type);
  z_histogram_dump(&(self->fwd_size), stream, z_human_size_d);
  fprintf(stream, "\n");

  fprintf(stream, "%s Body Size", msg_type);
  z_histogram_dump(&(self->body_size), stream, z_human_size_d);
  fprintf(stream, "\n");

  fprintf(stream, "%s Data Size", msg_type);
  z_histogram_dump(&(self->data_size), stream, z_human_size_d);
  fprintf(stream, "\n");
}

void z_ipc_msg_stats_add (z_ipc_msg_stats_t *self,
                          const z_ipc_msg_head_t *msg_head)
{
  z_histogram_add(&(self->fwd_size), msg_head->fwd_length);
  z_histogram_add(&(self->body_size), msg_head->body_length);
  z_histogram_add(&(self->data_size), msg_head->data_length);
}

void z_ipc_msg_stats_add_latency (z_ipc_msg_stats_t *self,
                                  uint64_t latency)
{
  z_histogram_add(&(self->latency), latency);
}

/* ============================================================================
 *  ipc-msg reader helpers
 */
enum z_msg_state {
  Z_MSG_READ_HEAD,
  Z_MSG_READ_FWD,
  Z_MSG_READ_BODY,
  Z_MSG_READ_DATA,
  Z_MSG_READ_FAILURE,
};

static int __ipc_msg_read_data (z_ipc_msg_reader_t *self,
                                const z_ipc_msg_head_t *msg_head,
                                const z_io_seq_vtable_t *io_proto,
                                const z_ipc_msg_protocol_t *proto,
                                void *io_data,
                                void *proto_data)
{
  struct iovec iov[3];
  struct iovec *p;
  uint32_t buflen;
  ssize_t rtotal;
  ssize_t rd;
  int iovcnt;

  p = iov;
  rtotal = 0;
  buflen = self->buflen;
  switch (self->state) {
    case Z_MSG_READ_FWD:
      p->iov_base = self->fwd + buflen;
      p->iov_len  = msg_head->fwd_length - buflen;
      rtotal += p->iov_len;
      buflen = 0;
      p++;
    case Z_MSG_READ_BODY:
      p->iov_base = self->body + buflen;
      p->iov_len  = msg_head->body_length - buflen;
      rtotal += p->iov_len;
      buflen = 0;
      p++;
    case Z_MSG_READ_DATA:
      p->iov_base = self->data + buflen;
      p->iov_len  = msg_head->data_length - buflen;
      rtotal += p->iov_len;
      buflen = 0;
      p++;
  }

  iovcnt = p - iov;
  rd = io_proto->readv(io_data, iov, iovcnt);
  if (Z_UNLIKELY(rd < 0)) {
    self->state = Z_MSG_READ_FAILURE;
    return(-1);
  }

  self->rdmore = (rd == rtotal);
  self->read_calls++;
  for (p = iov; iovcnt--; ++p) {
    if (rd < p->iov_len) {
      self->buflen += rd;
      return(1);
    }
    self->buflen = 0;
    rd -= p->iov_len;
    self->state++;
  }

  if (Z_UNLIKELY(proto->publish(self, msg_head, proto_data) < 0)) {
    self->state = Z_MSG_READ_FAILURE;
    return(-1);
  }
  self->state = Z_MSG_READ_HEAD;
  self->read_calls = 0;
  self->buflen = 0;
  return(0);
}

static ssize_t __ipc_msg_parse (z_ipc_msg_reader_t *self,
                                const uint8_t *buffer,
                                ssize_t buflen,
                                const z_io_seq_vtable_t *io_proto,
                                const z_ipc_msg_protocol_t *proto,
                                void *udata)
{
  z_ipc_msg_head_t msg_head;
  z_io_buf_t iobuf;
  uint8_t head[6];
  int hlen;

  if (Z_UNLIKELY(buflen < 2)) goto _incomplete_head;
  head[0] = (buffer[0] >> 4) & 15;          /* package type */
  head[1] = ((buffer[0] >> 2) & 3) + 1;     /* msg-type length bytes */
  head[2] = (buffer[0] & 3);                /* fwd length bytes */
  head[3] = ((buffer[1] >> 5) & 7) + 1;     /* msg-id length bytes */
  head[4] = (buffer[1] >> 3) & 3;           /* body length bytes */
  head[5] = (buffer[1] & 7);                /* data length bytes */

  hlen = 2 + head[1] + head[2] + head[3] + head[4] + head[5];
  if (Z_UNLIKELY(buflen < hlen)) goto _incomplete_head;

  buffer += 2;
  z_uint32_decode(buffer, head[1], &(msg_head.msg_type));    buffer += head[1];
  z_uint64_decode(buffer, head[3], &(msg_head.msg_id));      buffer += head[3];
  z_uint32_decode(buffer, head[2], &(msg_head.fwd_length));  buffer += head[2];
  z_uint32_decode(buffer, head[4], &(msg_head.body_length)); buffer += head[4];
  z_uint32_decode(buffer, head[5], &(msg_head.data_length)); buffer += head[5];

  if (Z_UNLIKELY(proto->alloc(self, head[0], &msg_head, udata) < 0)) {
    self->state = Z_MSG_READ_FAILURE;
    return(-1);
  }
  self->state = Z_MSG_READ_FWD;
  self->buflen = 0;

  iobuf.buffer = buffer;
  iobuf.buflen = buflen - hlen;
  if (__ipc_msg_read_data(self, &msg_head, &z_io_buf_vtable, proto, &iobuf, udata)) {
    memcpy(&(self->h.hmsg), &msg_head, sizeof(z_ipc_msg_head_t));
  }
  return(iobuf.buflen);

_incomplete_head:
  self->state = Z_MSG_READ_HEAD;
  memcpy(self->h.hbuf, buffer, buflen);
  self->buflen = buflen;
  return(0);
}

#define __IPC_MSG_BUFLEN      (512)

static void __msgbuf_read_new (z_ipc_msg_reader_t *self,
                               const z_io_seq_vtable_t *io_proto,
                               const z_ipc_msg_protocol_t *proto,
                               void *udata)
{
  uint8_t buffer[__IPC_MSG_BUFLEN];
  ssize_t avail;
  uint8_t *pbuf;
  int read_more;
  ssize_t rd;

  pbuf = buffer;
  avail = __IPC_MSG_BUFLEN;
  if (self->buflen != 0) {
    memcpy(pbuf, self->h.hbuf, self->buflen);
    pbuf += self->buflen;
    avail -= self->buflen;
  }

  self->read_calls = 1;
  self->first_read_ts = z_time_monotonic();
  rd = io_proto->read(udata, pbuf, avail);
  if (Z_UNLIKELY(rd < 0)) {
    self->state = Z_MSG_READ_FAILURE;
    return;
  }

  read_more = (rd == avail);
  rd += self->buflen;
  pbuf = buffer;
  while (rd != 0) {
    ssize_t rd_next;
    rd_next = __ipc_msg_parse(self, pbuf, rd, io_proto, proto, udata);
    pbuf += (rd - rd_next);
    rd = rd_next;
  }

  self->rdmore = read_more;
}

void z_ipc_msg_reader_open (z_ipc_msg_reader_t *self) {
  self->state = Z_MSG_READ_HEAD;
  self->buflen = 0;
}

void z_ipc_msg_reader_close (z_ipc_msg_reader_t *self) {
}

int z_ipc_msg_reader_read (z_ipc_msg_reader_t *self,
                           const z_io_seq_vtable_t *io_proto,
                           const z_ipc_msg_protocol_t *proto,
                           void *udata)
{
  do {
    self->rdmore = 0;
    switch (self->state) {
      case Z_MSG_READ_HEAD:
        __msgbuf_read_new(self, io_proto, proto, udata);
        break;
      case Z_MSG_READ_FWD:
      case Z_MSG_READ_BODY:
      case Z_MSG_READ_DATA:
        __ipc_msg_read_data(self, &(self->h.hmsg), io_proto, proto, udata, udata);
        break;
      case Z_MSG_READ_FAILURE:
        return(-1);
    }
  } while (self->rdmore);
  return(0);
}

/* ============================================================================
 *  ipc-msg helpers
 */
z_ipc_msg_buf_t *z_ipc_msg_buf_alloc (z_memory_t *memory) {
  z_ipc_msg_buf_t *msg;

  msg = z_memory_struct_alloc(memory, z_ipc_msg_buf_t);
  if (Z_MALLOC_IS_NULL(msg)) return(NULL);

  z_ipc_msg_buf_init(msg);
  return msg;
}

void z_ipc_msg_buf_free (z_ipc_msg_buf_t *msg, z_memory_t *memory) {
  z_dbuf_reader_t reader;

  z_dbuf_reader_init(&reader, memory, msg->head, msg->offset,
                     msg->index, msg->consumed);
  z_dbuf_reader_clear(&reader);

  z_memory_struct_free(memory, z_ipc_msg_buf_t, msg);
}

void z_ipc_msg_buf_init (z_ipc_msg_buf_t *self) {
  z_dlink_init(&(self->node));
  self->head = NULL;
  self->tail = NULL;
  self->bufsize = 0;
  self->consumed = 0;
  self->offset = 0;
  self->index = 0;
  self->write_calls = 0;
  self->req_ts = 0;
  self->resp_ts = 0;
}

void z_ipc_msg_buf_writer_open (z_ipc_msg_buf_t *self,
                                z_dbuf_writer_t *writer,
                                z_memory_t *memory)
{
  z_dbuf_writer_init(writer, memory);
}

void z_ipc_msg_buf_writer_close (z_ipc_msg_buf_t *self,
                                 z_dbuf_writer_t *writer)
{
  self->head = writer->head;
  self->tail = writer->tail;
  self->bufsize = writer->total;
}

int z_ipc_msg_buf_write_head (z_ipc_msg_buf_t *self,
                              z_dbuf_writer_t *writer,
                              uint8_t pkg_type,
                              const z_ipc_msg_head_t *head)
{
  uint8_t hbuf[24];
  uint8_t hlen[6];
  uint8_t *pbuf;

  hlen[0] = z_uint32_size(head->msg_type);
  hlen[1] = z_uint32_zsize(head->fwd_length);
  hlen[2] = z_uint64_size(head->msg_id);
  hlen[3] = z_uint32_zsize(head->body_length);
  hlen[4] = z_uint32_zsize(head->data_length);
  hlen[5] = 2 + hlen[0] + hlen[1] + hlen[2] + hlen[3] + hlen[4];

  pbuf = z_dbuf_writer_get(writer, hbuf, 24);
  *pbuf++ = (pkg_type << 4) | ((hlen[0] - 1) << 2) | hlen[1];
  *pbuf++ = ((hlen[2] - 1) << 5) | (hlen[3] << 3) | hlen[4];
  z_uint32_encode(pbuf, hlen[0], head->msg_type);    pbuf += hlen[0];
  z_uint32_encode(pbuf, hlen[2], head->msg_id);      pbuf += hlen[1];
  z_uint32_encode(pbuf, hlen[1], head->fwd_length);  pbuf += hlen[2];
  z_uint32_encode(pbuf, hlen[3], head->body_length); pbuf += hlen[3];
  z_uint32_encode(pbuf, hlen[4], head->data_length);
  return z_dbuf_writer_commit(writer, pbuf, hlen[5]);
}

void z_ipc_msg_buf_edit_head (z_ipc_msg_buf_t *self,
                              z_dbuf_writer_t *writer,
                              uint32_t body_length,
                              uint32_t data_length)
{
  uint8_t *buffer = writer->head->data;
  uint8_t hlen[5];

  hlen[0] = ((buffer[2] >> 2) & 3) + 1;     /* msg-type length bytes */
  hlen[1] = (buffer[2] & 3);                /* fwd length bytes */
  hlen[2] = ((buffer[3] >> 5) & 7) + 1;     /* msg-id length bytes */
  hlen[3] = (buffer[3] >> 3) & 3;           /* body length bytes */
  hlen[4] = (buffer[3] & 7);                /* data length bytes */

  buffer += 4 + hlen[0] + hlen[1] + hlen[2];
  if (body_length < 0xffffffff) z_uint32_encode(buffer, hlen[3], body_length);
  buffer += hlen[3];
  if (data_length < 0xffffffff) z_uint32_encode(buffer, hlen[4], data_length);
}

/* ============================================================================
 *  ipc-msg writer helpers
 */
void z_ipc_msg_writer_open (z_ipc_msg_writer_t *self) {
  z_dlink_init(&(self->head));
}

void z_ipc_msg_writer_close (z_ipc_msg_writer_t *self) {
}

void z_ipc_msg_writer_clear (z_ipc_msg_writer_t *self, z_memory_t *memory) {
  z_ipc_msg_buf_t *msg;
  fprintf(stderr, "CLOSE WRITER %p\n", self);
  z_dlink_del_for_each_entry(&(self->head), msg, z_ipc_msg_buf_t, node, {
    /* TODO proto->remove(self, msg, udata) */
    z_ipc_msg_buf_free(msg, memory);
  });
}

void z_ipc_msg_writer_push  (z_ipc_msg_writer_t *self,
                             z_ipc_msg_buf_t *msg)
{
  z_dlink_add_tail(&(self->head), &(msg->node));
}

int z_ipc_msg_writer_flush (z_ipc_msg_writer_t *self,
                            z_memory_t *memory,
                            const z_io_seq_vtable_t *io_proto,
                            const z_ipc_msg_protocol_t *proto,
                            void *udata)
{
  z_dbuf_reader_t reader;
  z_dbuf_iovec_t iovec;
  z_ipc_msg_buf_t *msg;
  ssize_t rd;

  if (z_dlink_is_empty(&(self->head)))
    return(0);

  msg = z_dlink_front_entry(&(self->head), z_ipc_msg_buf_t, node);
  z_dbuf_reader_init(&reader, memory, msg->head, msg->offset, msg->index, msg->consumed);
  z_dbuf_reader_get(&reader, &iovec);

  rd = io_proto->writev(udata, iovec.iov, iovec.count);
  if (rd < 0) {
    perror("writev()");
    return(-1);
  }

  z_dbuf_reader_remove(&reader, &iovec, rd);
  msg->head = reader.head;
  msg->bufsize -= rd;
  msg->offset = reader.offset;
  msg->index = reader.index;
  msg->consumed = reader.consumed;
  if (msg->head == NULL) {
    z_dlink_del(&(msg->node));
    proto->remove(self, msg, udata);
    z_ipc_msg_buf_free(msg, memory);
  }

  /*
   * merge dbuffers and writev
   * TODO proto->remove(self, msg, udata)
   */
  return(0);
}
