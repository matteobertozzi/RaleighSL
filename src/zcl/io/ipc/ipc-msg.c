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
#include <zcl/global.h>
#include <zcl/debug.h>
#include <zcl/time.h>
#include <zcl/ipc.h>
#include <zcl/fd.h>

#ifndef Z_IPC_MSG_RDBUF
  #define Z_IPC_MSG_RDBUF   512
#endif

#ifndef Z_IPC_MSG_RDBUF_SKIP
  #define Z_IPC_MSG_RDBUF_SKIP 2048
#endif

/* ============================================================================
 *  PRIVATE IPC msg-client head
 *  +------------------------------------------------------------------------+
 *  | pkg_type (6bits)                              | msg_type_bytes (2bits) |
 *  +------------------------------------------------------------------------+
 *  | msg_id_bytes (3bits) | body_len_bytes (2bits) | blob_len_bytes (3bits) |
 *  +------------------------------------------------------------------------+
 *  | msg_type (1-4 bytes)                                                   |
 *  +------------------------------------------------------------------------+
 *  | msg_id (1-8 bytes)                                                     |
 *  +------------------------------------------------------------------------+
 *  | body_length (0-2 bytes)                                                |
 *  +------------------------------------------------------------------------+
 *  | blob_length (0-4 bytes)                                                |
 *  +------------------------------------------------------------------------+
 */
static int __ipc_msg_parse_head (z_ipc_msg_head_t *head,
                                 const uint8_t *hbuf,
                                 int hlen)
{
  register const uint8_t *pbuf[4];
  register uint8_t nbytes[4];
  int head_len = 2;

  pbuf[0]   = hbuf + 2;
  nbytes[0] = (hbuf[0] & 3) + 1;
  head_len += (hbuf[0] & 3) + 1;

  pbuf[1]   = hbuf + head_len;
  nbytes[1] = (hbuf[1] >> 5) + 1;
  head_len += (hbuf[1] >> 5) + 1;

  pbuf[2]   = hbuf + head_len;
  nbytes[2] = (hbuf[1] >> 3) & 3;
  head_len += (hbuf[1] >> 3) & 3;

  pbuf[3]   = hbuf + head_len;
  nbytes[3] = (hbuf[1] & 7);
  head_len += (hbuf[1] & 7);

  if (Z_UNLIKELY(hlen < head_len))
    return(-1);

  z_uint32_decode(pbuf[0], nbytes[0], &(head->msg_type));
  z_uint64_decode(pbuf[1], nbytes[1], &(head->msg_id));
  z_uint16_decode(pbuf[2], nbytes[2], &(head->body_length));
  z_uint32_decode(pbuf[3], nbytes[3], &(head->blob_length));
  head->pkt_type = (hbuf[0] >> 2);

  return(head_len);
}

static int __ipc_msg_prepare_head (uint8_t *buf,
                                   const z_ipc_msg_head_t *head,
                                   uint16_t body_length,
                                   uint32_t blob_length)
{
  uint8_t h_msg_type = z_uint32_size(head->msg_type);
  uint8_t h_msg_id = z_uint64_size(head->msg_id);

  buf[0] = (head->pkt_type << 2) | (h_msg_type - 1);
  buf[1] = ((h_msg_id - 1) << 5) | (body_length << 3) | blob_length;
  z_uint32_encode(buf + 2,              h_msg_type, head->msg_type);
  z_uint64_encode(buf + 2 + h_msg_type, h_msg_id, head->msg_id);
  return(2 + h_msg_type + h_msg_id + body_length + blob_length);
}


/* ============================================================================
 *  PRIVATE IPC msg-client write methods
 */
#define __MSGQ_FLUSH_IOVS     16
static int __ipc_msg_flush (z_iopoll_entity_t *entity, z_stailq_t *msgq) {
  struct iovec iovs[__MSGQ_FLUSH_IOVS];
  z_memory_t *memory;
  z_ipc_msg_t *msg;
  uint64_t wtime;
  ssize_t wr;
  int iovcnt;

  /* Compose the iovs */
  iovcnt = 0;
  z_stailq_for_each_entry(msgq, msg, z_ipc_msg_t, node, {
    z_dbuf_reader_t reader;
    z_dbuf_reader_init(&reader, NULL, msg->hskip ? msg->dnode.next : &(msg->dnode),
                       msg->rhead, msg->hoffset);
    iovcnt += z_dbuf_reader_iovs(&reader, iovs + iovcnt, __MSGQ_FLUSH_IOVS - iovcnt);
    if (iovcnt == __MSGQ_FLUSH_IOVS)
      break;
  });

  /* Send the data to the client */
  Z_ASSERT(iovcnt > 0, "Expected at least 1 iovcnt got %d", iovcnt);
  wr = z_fd_writev(entity->fd, iovs, iovcnt);
  if (Z_UNLIKELY(wr < 0)) {
    Z_LOG_ERRNO_WARN("client %d writev()", entity->fd);
    return(-1);
  }
  //Z_LOG_TRACE("writev() iovcnt=%d wr=%zd", iovcnt, wr);

  /* Remove the sent messages */
  wtime = z_time_micros();
  memory = z_global_memory();
  z_stailq_for_each_safe_entry(msgq, msg, z_ipc_msg_t, node, {
    z_dbuf_reader_t reader;

    z_dbuf_reader_init(&reader, memory, msg->hskip ? msg->dnode.next : &(msg->dnode),
                       msg->rhead, msg->hoffset);

    wr = z_dbuf_reader_remove(&reader, wr);
    if (wr == 0) {
      if (reader.head == NULL) {
        z_stailq_remove(msgq);
        z_iopoll_add_latencies(entity, msg->latency, wtime - msg->wtime);
        z_memory_struct_free(memory, z_ipc_msg_t, msg);
        return(0);
      }

      if (reader.head != &(msg->dnode)) {
        msg->hskip = 1;
        msg->dnode.next = reader.head;
      }
      msg->hoffset = reader.poffset;
      msg->rhead = reader.phead;
      return(0);
    }

    z_stailq_remove(msgq);
    z_iopoll_add_latencies(entity, msg->latency, wtime - msg->wtime);
    z_memory_struct_free(memory, z_ipc_msg_t, msg);
  });
  return(0);
}

/* ============================================================================
 *  PRIVATE IPC msg-client read methods
 */
static int __ipc_msg_exec (z_ipc_msg_client_t *self,
                           const z_ipc_msg_protocol_t *proto,
                           z_ipc_msg_head_t *head)
{
  uint32_t latency;

  latency = (z_time_micros() & 0xffffffff) - self->rdbuf.first_rd_usec;
  Z_LOG_TRACE("read latency %uusec rdcount %u", latency, self->rdbuf.rdcount);

  if (proto->msg_exec(self, head) < 0)
    return(-1);

  memset(&(self->rdbuf), 0, sizeof(z_ipc_msg_rdbuf_t));
  return(0);
}

static ssize_t __ipc_msg_client_read_buf (z_ipc_msg_client_t *self,
                                          const z_ipc_msg_protocol_t *proto,
                                          const uint8_t *buffer,
                                          size_t rd)
{
  z_ipc_msg_rdbuf_t *rdbuf = &(self->rdbuf);
  const uint8_t *pbuffer = buffer;
  z_ipc_msg_head_t head;
  int r;

  /* check for min head length */
  if (Z_UNLIKELY(rd < 4)) {
    memcpy(rdbuf->head, buffer, rd);
    return(0);
  }

  /* Try to parse IPC-Header */
  r = __ipc_msg_parse_head(&head, buffer, rd & 0xffff);
  if (r < 0) {
    memcpy(rdbuf->head, buffer, rd);
    return(0);
  }

  /* skip the head */
  rdbuf->hlength = r;
  rdbuf->rd_data = 1;
  pbuffer += r;
  rd -= r;

  /* Ask the client to allocate the message */
  r = proto->msg_alloc(self, &head);
  if (Z_UNLIKELY(r < 0))
    return(-1);

  rdbuf->body_remaining = head.body_length;
  rdbuf->blob_remaining = head.blob_length;

  /* Try to copy the available body */
  r = z_min(rd, head.body_length);
  if (rdbuf->body_buffer != NULL) {
    memcpy(rdbuf->body_buffer, pbuffer, r);
    rdbuf->body_buffer += r;
  }
  rdbuf->body_remaining -= r;
  pbuffer += r;
  rd -= r;

  if (rdbuf->body_remaining != 0) {
    memcpy(rdbuf->head, buffer, rdbuf->hlength);
    return(rd);
  }

  /* If we have the full body, parse it */
  if (proto->msg_parse != NULL && !rdbuf->parsed) {
    r = proto->msg_parse(self, &head);
    if (Z_UNLIKELY(r < 0))
      return(-1);
    rdbuf->parsed = 1;
  }

  /* Try to copy the available blob */
  r = z_min(rd, head.blob_length);
  if (rdbuf->body_buffer != NULL) {
    memcpy(rdbuf->blob_buffer, pbuffer, r);
    rdbuf->blob_buffer += r;
  }
  rdbuf->blob_remaining -= r;
  rd -= r;

  /* If we have the full blob, execute it */
  if (rdbuf->blob_remaining == 0) {
    if (__ipc_msg_exec(self, proto, &head) < 0)
      return(-1);
  } else {
    memcpy(rdbuf->head, buffer, rdbuf->hlength);
  }
  return(rd);
}

static ssize_t __ipc_msg_client_read (z_ipc_msg_client_t *self, int *read_more)
{
  z_ipc_msg_rdbuf_t *rdbuf = &(self->rdbuf);
  struct iovec iov[2];
  ssize_t rd, n;

  iov[0].iov_base = rdbuf->body_buffer;
  iov[0].iov_len  = rdbuf->body_remaining;
  iov[1].iov_base = rdbuf->blob_buffer;
  iov[1].iov_len  = rdbuf->blob_remaining;

  rdbuf->rdcount++;
  rd = z_fd_readv(Z_IOPOLL_ENTITY_FD(self), iov, 2);
  if (Z_UNLIKELY(rd < 0))
    return(-1);

  *read_more = (rd == (rdbuf->body_remaining + rdbuf->blob_remaining));

  n = z_min(rd, rdbuf->body_remaining);
  rdbuf->body_buffer += n;
  rdbuf->body_remaining -= n;

  n = z_min(rd - n, rdbuf->blob_remaining);
  rdbuf->blob_remaining -= n;
  rdbuf->blob_buffer += n;
  return(rd);
}

static ssize_t __ipc_msg_client_skip (z_ipc_msg_client_t *self, int *read_more)
{
  /* The message was discarded, drain the buffer */
  z_ipc_msg_rdbuf_t *rdbuf = &(self->rdbuf);
  const int fd = Z_IOPOLL_ENTITY_FD(self);
  uint8_t buffer[Z_IPC_MSG_RDBUF_SKIP];
  ssize_t rdtotal = 0;
  size_t required;
  uint32_t n;

  rdbuf->rdcount++;
  required = (rdbuf->body_remaining + rdbuf->blob_remaining);
  while (required >= Z_IPC_MSG_RDBUF_SKIP && *read_more) {
    rdbuf->rdcount++;
    ssize_t rd = z_fd_read(fd, buffer, Z_IPC_MSG_RDBUF_SKIP);
    if (Z_UNLIKELY(rd < 0)) {
      return(rd);
    }
    rdtotal += rd;
    required -= rd;
    *read_more = (rd == Z_IPC_MSG_RDBUF_SKIP);
  }

  if (required > 0 && *read_more) {
    rdbuf->rdcount++;
    ssize_t rd = z_fd_read(fd, buffer, required);
    if (Z_UNLIKELY(rd < 0)) {
      return(rd);
    }
    rdtotal += rd;
    *read_more = (rd == required);
  }

  n = z_min(rdtotal, rdbuf->body_remaining);
  rdbuf->body_remaining -= n;
  rdbuf->blob_remaining -= z_min(rdtotal - n, rdbuf->blob_remaining);
  return(rdtotal);
}

/* ============================================================================
 *  PUBLIC IPC msg-client read methods
 */
int z_ipc_msg_client_read (z_ipc_msg_client_t *self,
                           const z_ipc_msg_protocol_t *proto)
{
  z_ipc_msg_rdbuf_t *rdbuf = &(self->rdbuf);
  int read_more = 1;

  do {
    if (rdbuf->rd_data) {
      z_ipc_msg_head_t head;
      ssize_t rd;

      if (rdbuf->body_buffer == NULL && rdbuf->blob_buffer == NULL) {
        /* Skip the data */
        rd = __ipc_msg_client_skip(self, &read_more);
      } else {
        /* Fetch the data */
        rd = __ipc_msg_client_read(self, &read_more);
      }

      if (Z_UNLIKELY(rd < 0))
        return(-1);

      if (rdbuf->body_remaining != 0)
        break;

      /* Parse the header */
      __ipc_msg_parse_head(&head, rdbuf->head, rdbuf->hlength);

      /* If we have the full body, parse it (TODO: called more then once) */
      if (proto->msg_parse != NULL && !rdbuf->parsed) {
        if (proto->msg_parse(self, &head) < 0)
          return(-1);
        rdbuf->parsed = 1;
      }

      /* If we have the full blob, execute it */
      if (rdbuf->blob_remaining == 0) {
        if (__ipc_msg_exec(self, proto, &head) < 0)
          return(-1);
      }
    } else {
      uint8_t buffer[Z_IPC_MSG_RDBUF];
      uint8_t *pbuffer = buffer;
      ssize_t rd;

      if (rdbuf->hlength > 0) {
        pbuffer = buffer + rdbuf->hlength;
        memcpy(buffer, rdbuf->head, rdbuf->hlength);
      } else {
        rdbuf->first_rd_usec = z_time_micros() & 0xffffffff;
      }

      /* Try to read IPC-Header */
      rdbuf->rdcount++;
      rd = z_fd_read(Z_IOPOLL_ENTITY_FD(self), pbuffer, Z_IPC_MSG_RDBUF - rdbuf->hlength);
      if (Z_UNLIKELY(rd < 0))
        return(-1);

      read_more = (rd == (Z_IPC_MSG_RDBUF - rdbuf->hlength));
      do {
        /* Try to parse the new message */
        rd = __ipc_msg_client_read_buf(self, proto, pbuffer, rd);
        if (Z_UNLIKELY(rd < 0))
          return(-1);

        pbuffer += rd;
      } while (rd != 0 && !rdbuf->rd_data);
    }
  } while (read_more);
  return(0);
}

/* ============================================================================
 *  PUBLIC IPC msg-client write methods
 */
void z_ipc_msg_client_push (z_ipc_msg_client_t *self, z_ipc_msg_t *msg) {
  z_stailq_add(&(self->wmsgq), &(msg->node));
  z_ipc_client_set_data_available(self, 1);
}


int z_ipc_msg_client_flush (z_ipc_msg_client_t *self,
                            const z_ipc_msg_protocol_t *proto)
{
#if 0
  uint8_t buffer[4] = {0, 0, 0, 0};
  write(Z_IOPOLL_ENTITY_FD(self), buffer, 4);
  z_ipc_client_set_data_available(self, 0);
  return(0);
#else
  int has_more = 0;
  int r = 0;

  if (Z_LIKELY(z_stailq_is_not_empty(&(self->wmsgq)))) {
    r = __ipc_msg_flush(Z_IOPOLL_ENTITY(self), &(self->wmsgq));
    has_more = z_stailq_is_not_empty(&(self->wmsgq));
  }

  z_iopoll_set_writable(Z_IOPOLL_ENTITY(self), has_more);
  return(r);
#endif
}


/* ============================================================================
 *  PUBLIC IPC-Message
 */
z_ipc_msg_t *z_ipc_msg_writer_open (z_ipc_msg_t *self,
                                    const z_ipc_msg_head_t *head,
                                    z_dbuf_writer_t *writer,
                                    z_memory_t *memory)
{
  z_dbuf_node_t *node;
  int hsize;

  if (self == NULL) {
    self = z_memory_struct_alloc(memory, z_ipc_msg_t);
    if (Z_MALLOC_IS_NULL(self))
      return(NULL);
  }

  self->hoffset = 0;
  self->rhead = Z_DBUF_NO_HEAD;
  self->hskip = 0;

  node = &(self->dnode);
  node->next = NULL;
  z_dbuf_node_set_size(node, Z_IPC_MSG_IBUF);
  node->alloc = 0;
  memset(node->data, Z_DBUF_EOF, Z_IPC_MSG_IBUF);

  hsize = __ipc_msg_prepare_head(node->data + 1, head, 2, 4);
  node->data[0] = hsize & 0xff;

  z_dbuf_writer_init(writer, memory, node, Z_DBUF_NO_HEAD, Z_IPC_MSG_IBUF - hsize);
  return(self);
}

void z_ipc_msg_writer_close (z_ipc_msg_t *self,
                             z_dbuf_writer_t *writer,
                             uint16_t body_length,
                             uint32_t blob_length)
{
  z_dbuf_node_t *node = &(self->dnode);
  uint8_t ih_body_len;
  uint8_t ih_blob_len;
  uint8_t ih_msg_type;
  uint8_t ih_msg_id;
  uint8_t h_body_len;
  uint8_t h_blob_len;
  uint8_t diff;

  ih_msg_type = (node->data[1] & 3) + 1;
  ih_msg_id   = ((node->data[2] >> 5) & 7) + 1;
  ih_body_len = (node->data[2] >> 3) & 3;
  ih_blob_len = (node->data[2] & 7);

  h_body_len = (body_length > 0) ? z_uint16_size(body_length) : 0;
  h_blob_len = (blob_length > 0) ? z_uint32_size(blob_length) : 0;

  diff = z_abs_diff(h_body_len, ih_body_len) + z_abs_diff(h_blob_len, ih_blob_len);
  if (diff != 0) {
    uint8_t *phead;

    /* Move Header */
    phead = self->dnode.data + 0;
    phead = self->dnode.data + diff;
    memmove(phead, self->dnode.data, 3 + ih_msg_type + ih_msg_id);

    /* Fix Heder */
    phead[0] -= diff;
    phead[2]  = (phead[2] & 0xE0) | (h_body_len << 3) | h_blob_len;

    phead += 3 + ih_msg_type + ih_msg_id;
    z_uint16_encode(phead,              h_body_len, body_length);
    z_uint32_encode(phead + h_body_len, h_blob_len, blob_length);

    self->rhead = diff;
  } else {
    uint8_t *phead = self->dnode.data + 2 + ih_msg_type + ih_msg_id;
    z_uint16_encode(phead,              h_body_len, body_length);
    z_uint32_encode(phead + h_body_len, h_blob_len, blob_length);
  }
}

void z_ipc_msg_reader_open (z_ipc_msg_t *self,
                            z_dbuf_reader_t *reader,
                            z_memory_t *memory)
{
  z_dbuf_node_t *node = &(self->dnode);
  z_dbuf_reader_init(reader, memory, node, self->rhead, 0);
}