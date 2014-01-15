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

#include <zcl/global.h>
#include <zcl/coding.h>
#include <zcl/string.h>
#include <zcl/debug.h>
#include <zcl/ipc.h>
#include <zcl/fd.h>

#define Z_MSGBUF_VERSION      (0x0)
#define Z_MSGBUF_MAGIC        (0xaacc33d5)

static int __msgbuf_parse_head (z_ringbuf_t *ringbuf, uint8_t *version, uint32_t *size) {
  uint32_t magic;
  uint8_t buf[8];
  uint8_t *pbuf;

  pbuf = (uint8_t *)z_ringbuf_fetch(ringbuf, &buf, 8);
  if (Z_UNLIKELY(pbuf == NULL)) {
    return(0);
  }

  /* Parse header */
  *version = pbuf[0];
  *size    = pbuf[1] | pbuf[2] << 8 | pbuf[3] << 16;
  magic    = pbuf[4] | pbuf[5] << 8 | pbuf[6] << 16 | pbuf[7] << 24;

  /* Validate header */
  if (Z_UNLIKELY(*version > Z_MSGBUF_VERSION ||
                 magic != Z_MSGBUF_MAGIC ||
                 (8 + *size) > ringbuf->size))
  {
    Z_LOG_FATAL("Invalid ipc-message header: magic %"PRIx32" version %"PRIu8" size %"PRIu32,
                magic, *version, *size);
    return(-2);
  }

  return(1);
}

static void __msgbuf_build_head (z_ipc_msgbuf_t *self, uint8_t head[8], uint32_t size) {
  head[0] = self->version & 0xff;

  head[1] = size & 0xff;
  head[2] = (size >> 8) & 0xff;
  head[3] = (size >> 16) & 0xff;

  head[4] = Z_MSGBUF_MAGIC & 0xff;
  head[5] = (Z_MSGBUF_MAGIC >>  8) & 0xff;
  head[6] = (Z_MSGBUF_MAGIC >> 16) & 0xff;
  head[7] = (Z_MSGBUF_MAGIC >> 24) & 0xff;
}

/* ============================================================================
 *  PRIVATE IPC Output MsgBuf methods
 */
#define NODE_NBLOCKS      16
#define DUMP_NBLOCKS      64

struct msg {
  uint8_t *data;
  size_t size;
};

struct node {
  struct node *next;
  struct msg msgs[NODE_NBLOCKS];
};

struct buf {
  struct node *head;
  struct node *tail;
  int m_count;
  int m_offset;
  int d_offset;
};

static struct node *__node_alloc (z_ipc_msgbuf_t *self) {
  struct node *node;

  node = z_memory_struct_alloc(z_global_memory(), struct node);
  if (Z_MALLOC_IS_NULL(node))
    return(NULL);

  z_memzero(node, sizeof(struct node));
  return(node);
}

static void __node_free (struct node *node) {
  z_memory_t *memory = z_global_memory();
  int i;

  for (i = 0; i < NODE_NBLOCKS; ++i) {
    struct msg *msg = &(node->msgs[i]);
    if (Z_UNLIKELY(msg->data != NULL)) {
      z_memory_free(memory, msg->data);
      msg->data = NULL;
      msg->size = 0;
    }
  }

  z_memory_struct_free(z_global_memory(), struct node, node);
}

static void __ipc_outbuf_open (z_ipc_msgbuf_t *self) {
  self->obuffer.tail = NULL;
  self->obuffer.head = NULL;
  self->obuffer.m_count = 0;
  self->obuffer.m_offset = 0;
  self->obuffer.d_offset = 0;
  z_spin_alloc(&(self->obuffer.lock));
}

static void __ipc_outbuf_close (z_ipc_msgbuf_t *self) {
  struct node *node = (struct node *)self->obuffer.head;
  while (node != NULL) {
    struct node *next = node ->next;
    __node_free(node);
    node = next;
  }
  z_spin_free(&(self->obuffer.lock));
}

static int __ipc_outbuf_add (z_ipc_msgbuf_t *self, void *data, size_t size) {
  struct node *node = (struct node *)self->obuffer.tail;
  int require_new_node;
  size_t msg_idx;

  z_spin_lock(&(self->obuffer.lock));
  msg_idx = (self->obuffer.m_offset + self->obuffer.m_count) % NODE_NBLOCKS;
  require_new_node = (msg_idx == 0 && self->obuffer.m_count > 0);
  z_spin_unlock(&(self->obuffer.lock));

  if (node == NULL || require_new_node) {
    struct node *new_node;

    new_node = __node_alloc(self);
    if (Z_MALLOC_IS_NULL(new_node))
      return(-1);

    if (node == NULL) {
      self->obuffer.head = new_node;
    } else {
      node->next = new_node;
    }
    self->obuffer.tail = new_node;
    node = new_node;
  }

  z_spin_lock(&(self->obuffer.lock));
  ++(self->obuffer.m_count);
  z_spin_unlock(&(self->obuffer.lock));

  struct msg *msg = &(node->msgs[msg_idx]);
  msg->data = data;
  msg->size = size;
  return(0);
}

static int __ipc_outbuf_flush (z_ipc_msgbuf_t *self, int fd) {
  uint8_t heads[8 * DUMP_NBLOCKS];
  struct iovec iovs[2 * DUMP_NBLOCKS];
  struct iovec *iov = iovs;
  z_memory_t *memory;
  struct node *node;
  int m_offset;
  int count;
  int niovs;
  int i;

  z_spin_lock(&(self->obuffer.lock));
  niovs = z_min(self->obuffer.m_count, DUMP_NBLOCKS);
  z_spin_unlock(&(self->obuffer.lock));

  if (Z_UNLIKELY(niovs == 0))
    return(0);

  count = niovs;
  node = (struct node *)self->obuffer.head;
  m_offset = self->obuffer.m_offset;

  if (self->obuffer.d_offset > 0) {
    struct msg *msg = &(node->msgs[m_offset]);
    if (self->obuffer.d_offset < 8) {
      __msgbuf_build_head(self, heads, msg->size);
      iov->iov_base = heads + self->obuffer.d_offset;
      iov->iov_len = 8 - self->obuffer.d_offset;
      ++iov;
    }

    iov->iov_base = msg->data + (self->obuffer.d_offset - 8);
    iov->iov_len = msg->size - (self->obuffer.d_offset - 8);
    ++iov;

    ++m_offset;
    --count;
  }

  for (; node != NULL && count > 0; node = node->next) {
    for (i = m_offset; count > 0 && i < DUMP_NBLOCKS; ++i) {
      uint8_t *msg_head = heads + ((iov - iovs) * 8);
      struct msg *msg = &(node->msgs[i]);

      __msgbuf_build_head(self, msg_head, msg->size);
      iov->iov_base = msg_head;
      iov->iov_len = 8;
      ++iov;

      iov->iov_base = msg->data;
      iov->iov_len = msg->size;
      ++iov;

      --count;
    }
    m_offset = 0;
  }

  ssize_t wr = z_fd_writev(fd, iovs, iov - iovs);
  if (Z_UNLIKELY(wr < 0)) return(-1);

  count = niovs;
  node = (struct node *)self->obuffer.head;
  m_offset = self->obuffer.m_offset;
  memory = z_global_memory();
  ssize_t size = -(self->obuffer.d_offset);
  while (node != NULL && count > 0) {
    for (i = m_offset; count > 0 && i < DUMP_NBLOCKS; ++i) {
      struct msg *msg = &(node->msgs[i]);
      size += (8 + msg->size);
      if (size > wr) {
        z_spin_lock(&(self->obuffer.lock));
        self->obuffer.m_offset = i;
        self->obuffer.d_offset = (size - wr);
        self->obuffer.m_count = self->obuffer.m_count - (1 + (niovs - count));
        z_spin_unlock(&(self->obuffer.lock));
        return(0);
      }

      --count;

      /* Free Message */
      z_memory_free(memory, msg->data);
      msg->data = NULL;
      msg->size = 0;

      if (size == wr) {
        z_spin_lock(&(self->obuffer.lock));
        self->obuffer.m_offset = (i + 1) % NODE_NBLOCKS;
        self->obuffer.m_count = self->obuffer.m_count - (niovs - count);
        z_spin_unlock(&(self->obuffer.lock));
        return(0);
      }
    }

    z_spin_lock(&(self->obuffer.lock));
    self->obuffer.head = node->next;
    z_spin_lock(&(self->obuffer.lock));
    __node_free(node);

    node = (struct node *)self->obuffer.head;
    m_offset = 0;
  }
  return(0);
}

/* ============================================================================
 *  PUBLIC IPC MsgBuf methods
 */
int z_ipc_msgbuf_open (z_ipc_msgbuf_t *self, size_t isize) {
  z_ringbuf_alloc(&(self->ibuffer), isize);
  __ipc_outbuf_open(self);
  self->version = 0;
  return(0);
}

void z_ipc_msgbuf_close (z_ipc_msgbuf_t *self) {
  z_ringbuf_free(&(self->ibuffer));
  __ipc_outbuf_close(self);
}

int z_ipc_msgbuf_fetch (z_ipc_msgbuf_t *self,
                        z_iopoll_entity_t *client,
                        z_ipc_msg_parse_t msg_parse_func)
{
  uint8_t version;
  uint32_t size;
  int res;

  /* Fetch new data from the client */
  if (z_ringbuf_fd_fetch(&(self->ibuffer), Z_IOPOLL_ENTITY_FD(client)) <= 0)
    return(-1);

  /* Look for new messages */
  size_t avail = z_ringbuf_used(&(self->ibuffer));
  while ((res = __msgbuf_parse_head(&(self->ibuffer), &version, &size))) {
    struct iovec iov[2];

    avail -= 8;

    /* Is message data available? */
    if (avail < size)
      break;

    /* Keep track of the version used by the client, and use it for response */
    self->version = z_max(self->version, version);

    /* Parse message */
    z_ringbuf_rskip(&(self->ibuffer), 8);
    z_ringbuf_pop_iov(&(self->ibuffer), iov, size);
    if (msg_parse_func(client, iov))
      return(-3);

    avail -= size;
    z_ringbuf_rskip(&(self->ibuffer), size);
  }

  return(res);
}

int z_ipc_msgbuf_push (z_ipc_msgbuf_t *self, void *buf, size_t n) {
  return(__ipc_outbuf_add(self, buf, n));
}

int z_ipc_msgbuf_flush (z_ipc_msgbuf_t *self, z_iopoll_t *iopoll, z_iopoll_entity_t *entity) {
  if (__ipc_outbuf_flush(self, Z_IOPOLL_ENTITY_FD(entity)))
    return(-1);
  z_iopoll_set_writable(iopoll, entity, self->obuffer.m_count > 0);
  return(0);
}
