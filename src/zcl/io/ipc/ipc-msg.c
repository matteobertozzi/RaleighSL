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
#include <zcl/memory.h>
#include <zcl/writer.h>
#include <zcl/debug.h>
#include <zcl/time.h>
#include <zcl/ipc.h>
#include <zcl/mem.h>
#include <zcl/fd.h>

/* ============================================================================
 *  PRIVATE Message
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
    z_dbuf_reader_open(&reader, NULL, msg->hskip ? msg->dnode.next : &(msg->dnode),
                       msg->rhead, msg->hoffset);
    iovcnt += z_dbuf_reader_get_iovs(&reader, iovs + iovcnt, __MSGQ_FLUSH_IOVS - iovcnt);
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

    z_dbuf_reader_open(&reader, memory, msg->hskip ? msg->dnode.next : &(msg->dnode),
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
      msg->hoffset = reader.hoffset;
      msg->rhead = reader.rhead;
      return(0);
    }

    z_stailq_remove(msgq);
    z_iopoll_add_latencies(entity, msg->latency, wtime - msg->wtime);
    z_memory_struct_free(memory, z_ipc_msg_t, msg);
  });
  return(0);
}

/* ============================================================================
 *  PRIVATE IPC-Message
 */
#define __IPC_MSG_VERSION   (0)
#define __IPC_MIN_HEAD      (4)
#define __IPC_BUF_SIZE      (256)

static int __ipc_msg_parse_head (z_ipc_msg_client_t *client,
                                 z_ipc_msg_alloc_t msg_alloc_func,
                                 z_memslice_t *buffer)
{
  uint8_t h_msg_len, h_msg_type, h_req_id, head_size;
  z_ipc_msg_head_t h;
  void *msg_ctx;

  if (Z_UNLIKELY(buffer->size < __IPC_MIN_HEAD))
    return(0);

  h_msg_len  = buffer->data[0] & 7;
  h_msg_type = (buffer->data[1] >> 5) & 7;
  h_req_id   = (buffer->data[1] >> 2) & 7;
  h.req_type = buffer->data[1] & 3;

  head_size = 2 + h_msg_len + h_msg_type + h_req_id;
  if (Z_UNLIKELY(buffer->size < head_size))
    return(0);

  z_uint32_decode(buffer->data + 2, h_msg_len, &(h.msg_len));
  buffer->data += 2 + h_msg_len;
  z_uint64_decode(buffer->data, h_msg_type, &(h.msg_type));
  buffer->data += h_msg_type;
  z_uint64_decode(buffer->data, h_req_id, &(h.req_id));
  buffer->data += h_req_id;
  buffer->size -= head_size;

  msg_ctx = msg_alloc_func(client, &h);
  if (Z_UNLIKELY(msg_ctx == NULL))
    return(-1);

  client->msgbuf.ibuffer.flags.msg_length = h.msg_len;
  client->msgbuf.ibuffer.msg_ctx = msg_ctx;
  return(1);
}

static int __ipc_msg_prepare_head (uint8_t *buf, const z_ipc_msg_head_t *head) {
  uint8_t h_msg_type = z_uint64_size(head->msg_type);
  uint8_t h_req_id = z_uint64_size(head->req_id);

  buf[0] = (__IPC_MSG_VERSION << 3) | head->msg_len;
  buf[1] = (h_msg_type << 5) | (h_req_id << 2) | head->req_type;
  buf += 2 + head->msg_len;
  z_uint64_encode(buf, h_msg_type, head->msg_type);   buf += h_msg_type;
  z_uint64_encode(buf, h_req_id, head->req_id);
  return(2 + head->msg_len + h_msg_type + h_req_id);
}

/* ============================================================================
 *  PUBLIC IPC-Message
 */
z_ipc_msg_t *z_ipc_msg_alloc (z_memory_t *memory, const z_ipc_msg_head_t *head) {
  z_dbuf_node_t *node;
  z_ipc_msg_t *msg;
  int hsize;

  msg = z_memory_struct_alloc(memory, z_ipc_msg_t);
  if (Z_MALLOC_IS_NULL(msg))
    return(NULL);

  msg->hoffset = 0;
  msg->rhead = Z_DBUF_NO_HEAD;
  msg->hskip = 0;

  node = &(msg->dnode);
  node->next = NULL;
  z_dbuf_node_set_size(node, Z_IPC_MSG_IBUF);
  node->alloc = 0;

  hsize = __ipc_msg_prepare_head(node->data + 1, head);
  node->data[0] = hsize & 0xff;
  node->data[hsize + 1] = 0xff;
  return(msg);
}

void z_ipc_msg_writer_open (z_ipc_msg_t *self,
                            z_dbuf_writer_t *writer,
                            z_memory_t *memory)
{
  z_dbuf_node_t *node = &(self->dnode);
  z_dbuf_writer_open(writer, memory, node, 0, Z_IPC_MSG_IBUF - 1 - node->data[0]);
}

void z_ipc_msg_writer_close (z_ipc_msg_t *self, z_dbuf_writer_t *writer) {
  z_dbuf_node_t *node = &(self->dnode);
  uint8_t ih_msg_len;
  uint8_t h_msg_len;

  ih_msg_len = (node->data[1] & 7);
  h_msg_len = z_uint32_size(writer->size);
  if (h_msg_len < ih_msg_len) {
    uint8_t *pbuf = self->dnode.data;
    uint8_t diff;

    diff = ih_msg_len - h_msg_len;
    pbuf[diff + 2] = pbuf[2];
    pbuf[diff + 1] = (__IPC_MSG_VERSION << 3) | h_msg_len;
    pbuf[diff + 0] = pbuf[0] - diff;

    z_uint32_encode(pbuf + diff + 3, h_msg_len, writer->size);
    self->rhead = diff;
  } else {
    z_uint32_encode(self->dnode.data + 3, ih_msg_len, writer->size);
  }
}

/* ============================================================================
 *  PUBLIC IPC-Msgbuf
 */
void z_ipc_msgbuf_open (z_ipc_msgbuf_t *self) {
  z_memzero(&(self->ibuffer), sizeof(self->ibuffer));
  z_stailq_init(&(self->omsgq));
  z_ticket_init(&(self->lock));
}

void z_ipc_msgbuf_close (z_ipc_msg_client_t *client,
                         z_ipc_msg_free_t msg_free)
{
  z_ipc_msgbuf_t *self = &(client->msgbuf);
  z_memory_t *memory = z_global_memory();
  z_ipc_msg_t *msg;
  z_stailq_for_each_safe_entry(&(self->omsgq), msg, z_ipc_msg_t, node, {
    z_stailq_remove(&(self->omsgq));
    z_dbuffer_clear(memory, &(msg->dnode));
    z_memory_struct_free(memory, z_ipc_msg_t, msg);
  });

  if (self->ibuffer.msg_ctx != NULL) {
    msg_free(client, self->ibuffer.msg_ctx);
  }
}

void z_ipc_msgbuf_push (z_ipc_msgbuf_t *self, z_ipc_msg_t *msg) {
  z_ticket_acquire(&(self->lock));
  z_stailq_add(&(self->omsgq), &(msg->node));
  z_ticket_release(&(self->lock));
}

int z_ipc_msg_client_fetch (z_ipc_msg_client_t *client,
                            z_ipc_msg_alloc_t msg_alloc_func,
                            z_ipc_msg_parse_t msg_parse_func,
                            z_ipc_msg_exec_t msg_exec_func)
{
  uint8_t buffer[__IPC_BUF_SIZE];
  z_memslice_t bufslice;

  z_memcpy(buffer, client->msgbuf.ibuffer.data, client->msgbuf.ibuffer.flags.bufsize);
  z_memslice_set(&bufslice, buffer, client->msgbuf.ibuffer.flags.bufsize);
  while (1) {
    size_t rdsize;
    ssize_t rd;

    rdsize = (__IPC_BUF_SIZE - bufslice.size);
    rd = z_fd_read(Z_IOPOLL_ENTITY_FD(client), buffer + bufslice.size, rdsize);
    if (Z_UNLIKELY(rd < 0)) break;

    z_memslice_set(&bufslice, buffer, bufslice.size + rd);
    do {
      z_memslice_t msgslice;
      uint32_t msg_consumed;
      void *msg_ctx;

      /* Parse message head */
      if ((msg_ctx = client->msgbuf.ibuffer.msg_ctx) == NULL) {
        int res = __ipc_msg_parse_head(client, msg_alloc_func, &bufslice);
        if (res <= 0) {
          if (res == 0)
            break;
          return(res);
        }
        msg_ctx = client->msgbuf.ibuffer.msg_ctx;
        /* TODO: Handle large messages */
      }

      /* Parse message body */
      msg_consumed = 0;
      z_memslice_set(&msgslice, bufslice.data,
                     z_min(bufslice.size, client->msgbuf.ibuffer.flags.msg_length));
      while (msgslice.size > 0) {
        uint32_t pre_bufsize = msgslice.size;

        int r = msg_parse_func(client, msg_ctx, &msgslice);
        if (Z_UNLIKELY(r != 0)) {
          if (r < 0)
            return(-1);

          z_memslice_set(&bufslice, msgslice.data, bufslice.size - msg_consumed);
          client->msgbuf.ibuffer.flags.bufsize = bufslice.size;
          client->msgbuf.ibuffer.flags.msg_length -= msg_consumed;
          z_memcpy_slice(client->msgbuf.ibuffer.data, &bufslice);
          return(0);
        }

        msg_consumed += (pre_bufsize - msgslice.size);
      }

      /* Reset the available buffer */
      z_memslice_set(&bufslice, msgslice.data, bufslice.size - msg_consumed);

      /* If the message is incomplete, try to fetch more */
      client->msgbuf.ibuffer.flags.msg_length -= msg_consumed;
      if (client->msgbuf.ibuffer.flags.msg_length != 0)
        break;

      /* Message completed */
      client->msgbuf.ibuffer.msg_ctx = NULL;

      /* Execute the message */
      z_atomic_inc(&(client->refs));
      if (Z_UNLIKELY(msg_exec_func(client, msg_ctx) < 0)) {
        z_atomic_dec(&(client->refs));
        return(-1);
      }

      /* try to parse another one */
    } while (bufslice.size >= __IPC_MIN_HEAD);

    if (rd < rdsize) {
      client->msgbuf.ibuffer.flags.bufsize = bufslice.size;
      z_memcpy_slice(client->msgbuf.ibuffer.data, &bufslice);
      break;
    }
    z_memmove(buffer, bufslice.data, bufslice.size);
  }
  return(0);
}

int z_ipc_msg_client_flush (z_ipc_msg_client_t *client) {
  int has_more = 0;
  int r = 0;

  z_ticket_acquire(&(client->msgbuf.lock));
  if (Z_LIKELY(z_stailq_is_not_empty(&(client->msgbuf.omsgq)))) {
    r = __ipc_msg_flush(Z_IOPOLL_ENTITY(client), &(client->msgbuf.omsgq));
    has_more = z_stailq_is_not_empty(&(client->msgbuf.omsgq));
  }
  z_ticket_release(&(client->msgbuf.lock));

  z_iopoll_set_writable(Z_IOPOLL_ENTITY(client), has_more);
  return(r);
}
