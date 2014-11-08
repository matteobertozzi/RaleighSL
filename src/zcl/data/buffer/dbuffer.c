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

#include <zcl/dbuffer.h>
#include <zcl/memutil.h>
#include <zcl/debug.h>

#define __DBUF_EOF          0xff
#define __DBUF_REF          0xfe

#define Z_DBUF_NODE_MIN                 (64)
#define Z_DBUF_NODE_MAX                 (256)
#define Z_DBUF_NODE_OVERHEAD            (sizeof(z_dbuf_node_t) - Z_DBUF_IBUFLEN)

#define __dbuf_node_pend(node)          ((node)->data + z_dbuf_node_size(node))
#define __dbuf_node_tail(node, avail)   (__dbuf_node_pend(node) - (avail))

#define __dbuf_node_size(size)          (Z_DBUF_NODE_OVERHEAD + (size))

/* ============================================================================
 *  PUBLIC Data-Buffer Node
 */
void z_dbuffer_clear (z_memory_t *memory, z_dbuf_node_t *head) {
  while (head != NULL) {
    z_dbuf_node_t *next = head->next;
    if (head->alloc) {
      z_memory_free(memory, z_dbuf_node_t, head,
                    __dbuf_node_size(z_dbuf_node_size(head)));
    }
    head = next;
  }
}

/* ============================================================================
 *  PRIVATE dbuf-writer methods
 */
static z_dbuf_node_t *__dbuf_node_alloc (z_dbuf_writer_t *self, int size) {
  z_dbuf_node_t *node;

  /* Allocate new node */
  size  = (size > Z_DBUF_NODE_MAX) ? Z_DBUF_NODE_MAX : z_align_up(size, Z_DBUF_NODE_MIN);
  node = z_memory_alloc(self->memory, z_dbuf_node_t, size);
  if (Z_MALLOC_IS_NULL(node))
    return(NULL);

  /* Initialize node */
  node->next = NULL;
  node->size  = (size - Z_DBUF_NODE_OVERHEAD) >> 1;
  node->alloc = 1;
  memset(node->data, __DBUF_EOF, size - Z_DBUF_NODE_OVERHEAD);
  return(node);
}

static z_dbuf_node_t *__dbuf_writer_add_node (z_dbuf_writer_t *self, int size) {
  z_dbuf_node_t *node;

  node = __dbuf_node_alloc(self, size);
  if (Z_MALLOC_IS_NULL(node))
    return(NULL);

  /* Initialize writer */
  self->tail->next = node;
  self->tail  = node;
  self->pbuf  = node->data;
  self->phead = 0;
  self->avail = z_dbuf_node_size(node);
  return(node);
}

/* ============================================================================
 *  PUBLIC dbuf-writer methods
 */
int z_dbuf_writer_init (z_dbuf_writer_t *self,
                        z_memory_t *memory,
                        z_dbuf_node_t *tail,
                        uint8_t phead,
                        uint8_t avail)
{
  /* TODO: Add reserve, to allocate bigger chunk if we do small add */
  self->memory = memory;
  self->size = 0;

  if (tail == NULL) {
    tail = __dbuf_node_alloc(self, Z_DBUF_NODE_MIN);
    if (Z_MALLOC_IS_NULL(tail))
      return(1);

    self->avail = z_dbuf_node_size(tail);
    self->phead = __DBUF_EOF;
    self->pbuf = tail->data;
  } else {
    self->avail = avail;
    self->phead = phead;
    self->pbuf = __dbuf_node_tail(tail, avail);
  }

  self->tail = tail;
  self->head = tail;
  return(0);
}

uint8_t *z_dbuf_writer_next (z_dbuf_writer_t *self, uint8_t *buffer, uint8_t size) {
  return((self->avail < size) ? buffer : (self->pbuf + (self->phead == __DBUF_EOF)));
}

void z_dbuf_writer_move (z_dbuf_writer_t *self, uint8_t size) {
  z_dbuf_node_t *node = self->tail;
  uint8_t *pbuf = self->pbuf;
  if (self->phead != __DBUF_EOF) {
    self->avail -= size;
    node->data[self->phead] += size;
  } else {
    self->phead = (pbuf - node->data) & 0xff;
    self->avail -= size + 1;
    *pbuf++ = size;
  }
  self->pbuf += size;
  self->size += size;
}

uint8_t *z_dbuf_writer_commit (z_dbuf_writer_t *self, uint8_t *buffer, uint8_t size) {
  z_dbuf_node_t *node = self->tail;
  uint8_t *pbuf;

  if (size > self->avail) {
    node = __dbuf_writer_add_node(self, size);
    if (Z_UNLIKELY(node == NULL))
      return(NULL);

    pbuf = node->data;
    *pbuf++ = size;
    memcpy(pbuf, buffer, size);
    self->size += size;
    self->avail -= (size + 1);
    self->pbuf = node->data + 1 + size;
    self->phead = 0;
    return(pbuf);
  }

  pbuf = self->pbuf;
  if (self->phead != __DBUF_EOF) {
    node->data[self->phead] += size;
    self->pbuf  += size;
    self->avail -= size;
  } else {
    self->phead = (pbuf - node->data) & 0xff;
    *pbuf++ = size;
    self->pbuf  += (size + 1);
    self->avail -= (size + 1);
  }

  if (pbuf != buffer) {
    memcpy(pbuf, buffer, size);
  }

  self->size += size;
  return(pbuf);
}

int z_dbuf_writer_add (z_dbuf_writer_t *self, const void *data, size_t size) {
  const uint8_t *pdata = (const uint8_t *)data;
  z_dbuf_node_t *node = self->tail;

  while (size > 0) {
    uint8_t to_add;
    uint8_t *pbuf;

    if (self->avail <= (self->phead == Z_DBUF_NO_HEAD)) {
      node = __dbuf_writer_add_node(self, size);
      if (Z_UNLIKELY(node == NULL))
        return(-1);
    }

    pbuf = __dbuf_node_tail(node, self->avail);
    if (self->phead != Z_DBUF_NO_HEAD) {
      /* Update the header - inline size */
      to_add = z_min(size, self->avail);
      node->data[self->phead] += to_add;
    } else {
      /* Setup the header - inline size */
      self->phead = (pbuf - node->data) & 0xff;
      self->avail--;
      self->pbuf++;
      to_add = z_min(size, self->avail);
      *pbuf++ = to_add;
    }

    z_memcpy(pbuf, pdata, to_add);
    self->pbuf  += to_add;
    self->avail -= to_add;
    self->size  += to_add;
    pdata += to_add;
    size -= to_add;
  }
  return(0);
}

int z_dbuf_writer_add_ref (z_dbuf_writer_t *self, const z_memref_t *ref) {
  z_dbuf_node_t *node = self->tail;
  uint8_t *pbuf;

  if (self->avail <= sizeof(z_memref_t)) {
    node = __dbuf_writer_add_node(self, sizeof(z_memref_t));
    if (Z_UNLIKELY(node == NULL))
      return(-1);
  }

  pbuf = __dbuf_node_tail(node, self->avail);
  *pbuf++ = __DBUF_REF;
  self->avail -= 1 + sizeof(z_memref_t);
  self->size  += ref->slice.size;
  z_memref_acquire(Z_MEMREF(pbuf), ref);
  return(0);
}

/* ============================================================================
 *  PRIVATE Data-Buffer Reader
 */
static z_dbuf_node_t *__dbuf_reader_remove_head (z_dbuf_reader_t *self) {
  z_dbuf_node_t *node = self->head;

  self->head = node->next;
  self->phead = 0;
  self->poffset = 0;

  if (node->alloc) {
    z_memory_free(self->memory, z_dbuf_node_t, node,
                  __dbuf_node_size(z_dbuf_node_size(node)));
  }
  return(self->head);
}

/* ============================================================================
 *  PUBLIC Data-Buffer Reader
 */
void z_dbuf_reader_init (z_dbuf_reader_t *self,
                         z_memory_t *memory,
                         z_dbuf_node_t *head,
                         uint8_t phead,
                         int poffset)
{
  self->head = head;
  self->phead = phead;
  self->poffset = poffset;
  self->memory = memory;
}

int z_dbuf_reader_iovs (z_dbuf_reader_t *self, struct iovec *iovs, int iovcnt) {
  struct iovec *iov = iovs;
  z_dbuf_node_t *node;
  uint8_t *ptail;
  uint8_t *pbuf;
  int poffset;

  node = self->head;
  if (node == NULL)
    return(0);

  pbuf  = node->data + self->phead;
  ptail = __dbuf_node_pend(node);
  poffset = self->poffset;
  while (iovcnt > 0) {
    if (pbuf >= ptail || *pbuf == __DBUF_EOF) {
      node = node->next;
      if (node == NULL)
        break;

      pbuf = node->data;
      ptail = __dbuf_node_pend(node);
    }

    if (*pbuf == __DBUF_REF) {
      const z_memref_t *ref = Z_CONST_MEMREF(pbuf + 1);
      iov->iov_base = ref->slice.data;
      iov->iov_len  = ref->slice.size;
      pbuf += sizeof(z_memref_t) + 1;
    } else {
      iov->iov_base = pbuf + 1 + poffset;
      iov->iov_len  = *pbuf - poffset;
      pbuf += *pbuf + 1;
    }

    poffset = 0;
    iov++;
    iovcnt--;
  }
  return(iov - iovs);
}

size_t z_dbuf_reader_remove (z_dbuf_reader_t *self, size_t to_remove) {
  const z_dbuf_node_t *node;
  const uint8_t *ptail;

  node = self->head;
  if (node == NULL)
    return(0);

  ptail = __dbuf_node_pend(node);
  while (1) {
    const uint8_t *pbuf  = node->data + self->phead;
    int next_offset;
    int plen;

    if (pbuf >= ptail || *pbuf == __DBUF_EOF) {
      node = __dbuf_reader_remove_head(self);
      if (node == NULL)
        break;

      pbuf = node->data;
      ptail = __dbuf_node_pend(node);
      self->phead = 0;
    }

    if (to_remove == 0)
      break;

    if (*pbuf == __DBUF_REF) {
      const z_memref_t *ref = Z_CONST_MEMREF(pbuf + 1);
      plen = ref->slice.size - self->poffset;
      next_offset = sizeof(z_memref_t) + 1;
    } else {
      plen = *pbuf - self->poffset;
      next_offset = *pbuf + 1;
    }

    if (to_remove >= plen) {
      to_remove -= plen;
      self->phead += next_offset;
      self->poffset = 0;
    } else /* (to_remove < plen) */ {
      self->poffset += to_remove;
      to_remove = 0;
      break;
    }
  }
  return(to_remove);
}
