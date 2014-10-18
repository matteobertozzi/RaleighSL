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
      z_memory_free(memory, z_dbuf_node_t, head, __dbuf_node_size(z_dbuf_node_size(head)));
    }
    head = next;
  }
}

/* ============================================================================
 *  PUBLIC Data-Buffer Writer
 */
static z_dbuf_node_t *__dbuf_writer_add_node (z_dbuf_writer_t *self,
                                              size_t size)
{
  z_dbuf_node_t *node;

  /* Allocate new node */
  printf("ALLOC NODE REQUIRED %zu\n", size);
  size  = (size > Z_DBUF_NODE_MAX) ? Z_DBUF_NODE_MAX : z_align_up(size, Z_DBUF_NODE_MIN);
  node = z_memory_alloc(self->memory, z_dbuf_node_t, size);
  if (Z_MALLOC_IS_NULL(node))
    return(NULL);

  node->next  = NULL;
  node->size  = (size - Z_DBUF_NODE_OVERHEAD) >> 1;
  node->alloc = 1;
  node->data[0] = 0xff;

  /* Add to the writer tail */
  if (Z_LIKELY(self->tail != NULL))
    self->tail->next = node;
  self->tail = node;

  self->avail = size - Z_DBUF_NODE_OVERHEAD;
  self->whead = Z_DBUF_NO_HEAD;
  return(node);
}

int z_dbuf_writer_open (z_dbuf_writer_t *self,
                        z_memory_t *memory,
                        z_dbuf_node_t *head,
                        uint8_t whead,
                        uint8_t avail)
{
  /* TODO: Add reserve, to allocate bigger chunk if we do small add */
  self->memory = memory;
  self->size = 0;

  if (head == NULL) {
    head = __dbuf_writer_add_node(self, Z_DBUF_NODE_MIN);
    if (Z_UNLIKELY(head == NULL))
      return(-1);
    self->head = head;
    //printf("%p %p %u %u\n", self->head, self->tail, self->avail, self->whead);
  } else {
    self->tail = head;
    self->head = head;
    self->avail = avail;
    self->whead = whead;
  }
  return(0);
}

int z_dbuf_writer_add (z_dbuf_writer_t *self, const void *data, size_t size) {
  const uint8_t *pdata = (const uint8_t *)data;
  z_dbuf_node_t *node = self->tail;

  while (size > 0) {
    uint8_t to_add;
    uint8_t *pbuf;

    printf("AVAIL %u %u\n", self->avail, self->whead);
    if (self->avail <= (self->whead == Z_DBUF_NO_HEAD)) {
      node = __dbuf_writer_add_node(self, size);
      if (Z_UNLIKELY(node == NULL))
        return(-1);
    }

    pbuf = __dbuf_node_tail(node, self->avail);
    printf("AVAIL %u %u\n", self->avail, self->whead);
    printf(" - SIZE %u\n",  z_dbuf_node_size(node));
    printf(" - END %p TAIL %p %p\n", __dbuf_node_pend(node), pbuf, node->data);
    if (self->whead != Z_DBUF_NO_HEAD) {
      /* Update the header - inline size */
      to_add = z_min(size, self->avail);
      node->data[self->whead] += to_add;
    } else {
      /* Setup the header - inline size */
      self->whead = (pbuf - node->data) & 0xff;
      printf("WHEAD %u\n", self->whead);
      --(self->avail);
      to_add = z_min(size, self->avail);
      *pbuf++ = to_add;
    }

    z_memcpy(pbuf, pdata, to_add);
    self->avail -= to_add;
    self->size += to_add;
    pdata += to_add;
    size -= to_add;
  }

  /* Add EOF Marker */
  if (self->avail > 0) {
    *__dbuf_node_tail(node, self->avail) = 0xff;
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
  *pbuf++ = 0;
  self->avail -= 1 + sizeof(z_memref_t);
  self->size  += ref->slice.size;
  z_memref_acquire(Z_MEMREF(pbuf), ref);

  /* Add EOF Marker */
  if (Z_LIKELY(self->avail > 0)) {
    *(pbuf + sizeof(z_memref_t)) = 0xff;
  }
  return(0);
}

/* ============================================================================
 *  PUBLIC Data-Buffer Reader
 */
static uint8_t *__dbuf_node_fetch (uint8_t *pbuf, struct iovec *iov) {
  uint8_t isize = *pbuf++;

  Z_ASSERT(isize != 0xff, "unexpected EOF");
  if (isize > 0) {
    iov->iov_base = pbuf;
    iov->iov_len  = isize;
    pbuf += isize;
  } else {
    const z_memref_t *ref = Z_CONST_MEMREF(pbuf);
    iov->iov_base = ref->slice.data;
    iov->iov_len  = ref->slice.size;
    pbuf += sizeof(z_memref_t);
  }
  return(pbuf);
}

static z_dbuf_node_t *__dbuf_reader_remove_head (z_dbuf_reader_t *self) {
  z_dbuf_node_t *node = self->head;

  self->head = node->next;
  self->rhead = Z_DBUF_NO_HEAD;
  self->hoffset = 0;

  if (node->alloc) {
    z_memory_free(self->memory, z_dbuf_node_t, node, __dbuf_node_size(z_dbuf_node_size(node)));
  }
  return(self->head);
}

void z_dbuf_reader_open (z_dbuf_reader_t *self,
                         z_memory_t *memory,
                         z_dbuf_node_t *head,
                         uint8_t rhead,
                         uint32_t hoffset)
{
  self->head = head;
  self->rhead = rhead;
  self->hoffset = hoffset;
  self->memory = memory;
}

int z_dbuf_reader_get_iovs (z_dbuf_reader_t *self, struct iovec *iovs, int niovs) {
  z_dbuf_node_t *node = self->head;
  uint8_t *pbuf, *ebuf;
  int count = 0;

  Z_ASSERT(node != NULL, "expected head to be not NULL");

  pbuf = node->data;
  ebuf = __dbuf_node_pend(node);
  if (self->rhead != Z_DBUF_NO_HEAD) {
    struct iovec *iov = &(iovs[count++]);
    pbuf = __dbuf_node_fetch(pbuf + self->rhead, iov);
    iov->iov_base  = ((uint8_t *)iov->iov_base) + self->hoffset;
    iov->iov_len  -= self->hoffset;
  }

  while (1) {
    while (count < niovs && pbuf < ebuf && *pbuf != 0xff) {
      pbuf = __dbuf_node_fetch(pbuf, &(iovs[count++]));
    }

    node = node->next;
    if (count >= niovs || node == NULL)
      break;

    pbuf = node->data;
    ebuf = __dbuf_node_pend(node);
  }
  return(count);
}

size_t z_dbuf_reader_remove (z_dbuf_reader_t *self, size_t rm_size) {
  z_dbuf_node_t *node = self->head;
  uint8_t *pbuf, *ebuf;
  struct iovec iov;

  if (Z_UNLIKELY(rm_size == 0))
    return(0);

  pbuf = node->data;
  ebuf = __dbuf_node_pend(node);
  if (self->rhead != Z_DBUF_NO_HEAD) {
    z_memref_t *ref;
    pbuf += self->rhead;
    ref = (*pbuf == 0) ? Z_MEMREF(pbuf + 1) : NULL;
    pbuf = __dbuf_node_fetch(pbuf, &iov);
    iov.iov_base  = ((uint8_t *)iov.iov_base) + self->hoffset;
    iov.iov_len  -= self->hoffset;
    if (iov.iov_len >= rm_size) {
      self->hoffset += rm_size;
      if (iov.iov_len == rm_size) {
        if (pbuf < ebuf && *pbuf != 0xff) {
          /* More data available in this block */
          self->rhead = (pbuf - node->data) & 0xff;
          self->hoffset = 0;
        } else {
          /* Nothing more in this block */
          z_memref_release(ref);
          __dbuf_reader_remove_head(self);
        }
      }
      return(0);
    }
    rm_size -= iov.iov_len;
    z_memref_release(ref);
  }

  while (1) {
    while (rm_size > 0 && pbuf < ebuf && *pbuf != 0xff) {
      z_memref_t *ref = (*pbuf == 0) ? Z_MEMREF(pbuf + 1) : NULL;
      self->rhead = (pbuf - node->data) & 0xff;
      pbuf = __dbuf_node_fetch(pbuf, &iov);
      if (iov.iov_len >= rm_size) {
        self->hoffset += rm_size;
        if (iov.iov_len == rm_size) {
          if (pbuf < ebuf && *pbuf != 0xff) {
            /* More data available in this block */
            self->rhead = (pbuf - node->data) & 0xff;
            self->hoffset = 0;
          } else {
            /* Nothing more in this block */
            z_memref_release(ref);
            __dbuf_reader_remove_head(self);
          }
        }
        return(0);
      }
      rm_size -= iov.iov_len;
      z_memref_release(ref);
    }

    node = __dbuf_reader_remove_head(self);
    if (node == NULL || !rm_size)
      break;

    pbuf = node->data;
    ebuf = __dbuf_node_pend(node);
  }
  return(rm_size);
}
