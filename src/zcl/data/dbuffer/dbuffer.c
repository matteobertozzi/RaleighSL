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
#include <zcl/string.h>
#include <zcl/debug.h>

struct z_dbuf_vref {
  const z_ref_vtable_t *vtable;
  void *udata;
};

enum z_dbuf_rec_type {
  Z_DBUF_EOF     = 0,
  Z_DBUF_DATA    = 1,
  Z_DBUF_REF     = 2,
  Z_DBUF_REF_VEC = 3,
};

#define __dbuf_node_length(node)        ((node)->length + 1)
#define __dbuf_node_data_size(node)     (__dbuf_node_length(node) - sizeof(z_dbuf_node_t))

/* ============================================================================
 *  dbuf writer
 */
static z_dbuf_node_t *__dbuf_node_alloc (z_memory_t *memory, int size) {
  z_dbuf_node_t *node;

  /* Allocate new node */
  size = (size <= 64) ? 64 : (size <= 128) ? 128 : 256;
  node = z_memory_alloc(memory, z_dbuf_node_t, size);
  if (Z_MALLOC_IS_NULL(node))
    return(NULL);

  /* Initialize node */
  node->next    = NULL;
  node->length  = size - 1;
  memset(node->data, Z_DBUF_EOF, __dbuf_node_data_size(node));
  return(node);
}

void z_dbuf_writer_init (z_dbuf_writer_t *self, z_memory_t *memory) {
  self->type   = Z_DBUF_EOF;
  self->avail  = 0;
  self->size_offset = 0;
  self->data_offset = 0;
  self->total  = 0;
  self->tail = NULL;
  self->head = NULL;
  self->memory = memory;
}

int z_dbuf_writer_add_node (z_dbuf_writer_t *self, unsigned int size) {
  z_dbuf_node_t *node;

  node = __dbuf_node_alloc(self->memory, size);
  if (Z_MALLOC_IS_NULL(node))
    return(-1);

  /* Initialize writer */
  self->type  = Z_DBUF_EOF;
  self->avail = __dbuf_node_data_size(node);
  self->size_offset = 0;
  self->data_offset = 0;
  if (self->tail != NULL) {
    self->tail->next = node;
  } else {
    self->tail = node;
    self->head = node;
  }
  self->tail = node;
  return(0);
}

int z_dbuf_writer_add (z_dbuf_writer_t *self,
                       const void *buffer,
                       size_t size)
{
  const uint8_t *pbuf = Z_CONST_CAST(uint8_t, buffer);
  z_dbuf_node_t *node = self->tail;
  int avail;

  if (self->avail > 2) {
    uint8_t *pdata = node->data + self->data_offset;
    if (self->type != Z_DBUF_DATA) {
      self->type = Z_DBUF_DATA;
      self->avail -= 2;
      self->size_offset = self->data_offset + 1;
      self->data_offset += 2;
      avail = z_min(size, self->avail);
      *pdata++ = Z_DBUF_DATA;
      *pdata++ = avail;
    } else {
      avail = z_min(size, self->avail);
      node->data[self->size_offset] += avail;
    }

    memcpy(pdata, pbuf, avail);
    self->avail -= avail;
    self->data_offset += avail;
    self->total += avail;
    pbuf += avail;
    size -= avail;
  }

  while (size > 0) {
    if (z_dbuf_writer_add_node(self, size))
      break;

    node = self->tail;
    avail = z_min(size, self->avail - 2);
    node->data[0] = Z_DBUF_DATA;
    node->data[1] = avail;
    memcpy(node->data + 2, pbuf, avail);
    pbuf += avail;
    size -= avail;

    self->avail -= 2 + avail;
    self->size_offset = 1;
    self->data_offset = 2 + avail;
    self->total += avail;
  }
  return(size);
}

uint8_t *z_dbuf_writer_next (z_dbuf_writer_t *self, uint32_t *avail) {
  uint8_t *pdata = self->tail->data + self->data_offset;
  if (self->type != Z_DBUF_DATA) {
    *avail = (self->avail > 2) ? (self->avail - 2) : 0;
    return(pdata + 2);
  }
  *avail = self->avail;
  return(pdata);
}

uint8_t *z_dbuf_writer_get (z_dbuf_writer_t *self,
                            uint8_t *buffer,
                            uint32_t size)
{
  uint8_t *data = self->tail->data + self->data_offset;
  if (self->type != Z_DBUF_DATA) {
    size += 2;
    data += 2;
  }
  return((self->avail >= size) ? data : buffer);
}

int z_dbuf_writer_commit (z_dbuf_writer_t *self,
                          const void *buffer,
                          uint32_t size)
{
  const uint8_t *pbuf = Z_CONST_CAST(uint8_t, buffer);
  uint8_t *pdata = self->tail->data + self->data_offset;
  if (pbuf >= pdata && pbuf <= (pdata + self->avail)) {
    Z_ASSERT(size < 0xff, "dbuf node avail is always less then 255: %u", size);
    if (self->type == Z_DBUF_DATA) {
      self->tail->data[self->size_offset] += size;
    } else {
      self->type = Z_DBUF_DATA;
      self->size_offset = self->data_offset + 1;
      self->data_offset += 2;
      self->avail -= 2;
      pdata[0] = Z_DBUF_DATA;
      pdata[1] = size;
    }
    self->avail -= size;
    self->data_offset += size;
    self->total += size;
    return(0);
  }
  return z_dbuf_writer_add(self, buffer, size);
}

uint8_t *z_dbuf_writer_mark (z_dbuf_writer_t *self, const uint8_t size) {
  uint8_t *pbuf;

  if (self->avail < (size + (self->type != Z_DBUF_DATA ? 2 : 0))) {
    if (z_dbuf_writer_add_node(self, size))
      return(NULL);
  }

  pbuf = self->tail->data + self->data_offset;
  if (self->type != Z_DBUF_DATA) {
    self->type = Z_DBUF_DATA;
    self->avail -= 2;
    self->size_offset = self->data_offset + 1;
    self->data_offset += 2;
    *pbuf++ = Z_DBUF_DATA;
    *pbuf++ = size;
  } else {
    self->tail->data[self->size_offset] += size;
  }
  self->avail -= size;
  self->data_offset += size;
  self->total += size;
  return pbuf;
}

/* ============================================================================
 *  dbuf writer - ref
 */
int z_dbuf_writer_add_ref (z_dbuf_writer_t *self,
                           const z_ref_vtable_t *ref_vtable,
                           uint8_t *data,
                           uint32_t offset,
                           uint32_t length,
                           void *udata)
{
  const int size = 1 + sizeof(z_dbuf_vref_t) + sizeof(z_ref_data_t);
  z_dbuf_vref_t *vref;
  z_ref_data_t *ref;
  uint8_t *pbuf;

  if (self->avail < size) {
    if (z_dbuf_writer_add_node(self, size))
      return(-1);
  }

  pbuf = self->tail->data + self->data_offset;
  *pbuf++ = Z_DBUF_REF;

  vref = Z_CAST(z_dbuf_vref_t, pbuf);
  vref->vtable = ref_vtable;
  vref->udata  = udata;

  pbuf += sizeof(z_dbuf_vref_t);
  ref = Z_CAST(z_ref_data_t, pbuf);
  ref->offset = offset;
  ref->length = length;
  ref->data = data;

  self->type = Z_DBUF_REF;
  self->avail -= size;
  self->data_offset += size;
  self->total += length;
  return(0);
}

z_ref_data_t *z_dbuf_writer_next_refs (z_dbuf_writer_t *self,
                                       int *avail_refs)
{
  const int vref_size = 1 + sizeof(z_dbuf_vref_t);
  int avail = self->avail - vref_size;
  if (avail > 0) {
    uint8_t *pbuf = self->tail->data + self->data_offset + 1 + vref_size;
    *avail_refs = avail / sizeof(z_ref_data_t);
    return(Z_CAST(z_ref_data_t, pbuf));
  }
  *avail_refs = 0;
  return(NULL);
}

void z_dbuf_writer_commit_refs (z_dbuf_writer_t *self,
                                const z_ref_vtable_t *ref_vtable,
                                void *udata,
                                const z_ref_data_t *refs,
                                int count)
{
  const int size = 2 + sizeof(z_dbuf_vref_t) + (count * sizeof(z_ref_data_t));
  uint8_t *pbuf = self->tail->data + self->data_offset;
  z_dbuf_vref_t *vref;

  *pbuf++ = Z_DBUF_REF_VEC;
  *pbuf++ = (count & 0xff);

  vref = Z_CAST(z_dbuf_vref_t, pbuf);
  vref->vtable = ref_vtable;
  vref->udata  = udata;

  self->type = Z_DBUF_REF_VEC;
  self->avail -= size;
  self->data_offset += size;

  for (; count--; ++refs) {
    self->total += refs->length;
  }
}

/* ============================================================================
 *  dbuf reader
 */
void z_dbuf_reader_init (z_dbuf_reader_t *self,
                         z_memory_t *memory,
                         z_dbuf_node_t *head,
                         uint8_t offset,
                         uint8_t index,
                         uint32_t consumed)
{
  self->offset = offset;
  self->index = index;
  self->consumed = consumed;
  self->head = head;
  self->memory = memory;
}

/*
 * +-------------+---------+------------+
 * | Z_DBUF_DATA | u8 size | ...data... |
 * +-------------+---------+------------+
 */
static void __dbuf_data_get (z_dbuf_iovec_t *self) {
  uint8_t *pbuf = self->node->data + self->offset + 1;

  self->iov[self->count].iov_base = pbuf + 1 + self->consumed;
  self->iov[self->count].iov_len  = pbuf[0] - self->consumed;
  self->off[self->count] = self->offset;
  self->idx[self->count] = 0;
  self->nod[self->count] = self->node;
  self->vrf[self->count] = NULL;
  self->ref[self->count] = NULL;

  self->count++;
  self->offset += 2 + pbuf[0];
  self->consumed = 0;
}

/*
 * +------------+-------------+------------+
 * | Z_DBUF_REF | z_dbuf_vref | z_ref_data |
 * +------------+-------------+------------+
 */
static void __dbuf_ref_get (z_dbuf_iovec_t *self) {
  uint8_t *pbuf = self->node->data + self->offset + 1;
  z_dbuf_vref_t *vref;
  z_ref_data_t *ref;

  vref = Z_CAST(z_dbuf_vref_t, pbuf); pbuf += sizeof(z_dbuf_vref_t);
  ref  = Z_CAST(z_ref_data_t,  pbuf); pbuf += sizeof(z_ref_data_t);

  if (Z_UNLIKELY(vref->vtable == NULL)) {
    vref = NULL;
  }

  self->iov[self->count].iov_base = ref->data + ref->offset + self->consumed;
  self->iov[self->count].iov_len  = ref->length - self->consumed;
  self->off[self->count] = self->offset;
  self->idx[self->count] = 0;
  self->nod[self->count] = self->node;
  self->vrf[self->count] = vref;
  self->ref[self->count] = ref;

  self->count++;
  self->offset += 1 + sizeof(z_dbuf_vref_t) + sizeof(z_ref_data_t);
  self->consumed = 0;
}

/*
 * +----------------+----------+-------------+------------+---+------------+
 * | Z_DBUF_REF_VEC | u8 count | z_dbuf_vref | z_ref_data |...| z_ref_data |
 * +----------------+----------+-------------+------------+---+------------+
 */
static void __dbuf_ref_vec_get (z_dbuf_iovec_t *self) {
  uint8_t *pbuf = self->node->data + self->offset + 1;
  z_dbuf_vref_t *vref;
  z_ref_data_t *ref;
  int veclen;

  veclen = *pbuf++;
  vref = Z_CAST(z_dbuf_vref_t, pbuf);
  pbuf += sizeof(z_dbuf_vref_t);

  if (Z_UNLIKELY(vref->vtable == NULL)) {
    vref = NULL;
  }

  ref = Z_CAST(z_ref_data_t, pbuf) + self->index;
  while (self->count < Z_DBUF_NIOVS && self->index < veclen) {
    self->iov[self->count].iov_base = ref->data + ref->offset + self->consumed;
    self->iov[self->count].iov_len  = ref->length - self->consumed;
    self->off[self->count] = self->offset;
    self->idx[self->count] = self->index;
    self->nod[self->count] = self->node;
    self->vrf[self->count] = vref;
    self->ref[self->count] = ref;

    ref++;
    self->count++;
    self->index++;
    self->consumed = 0;
  }

  if (self->index == veclen) {
    self->offset += 2 + sizeof(z_dbuf_vref_t) + (veclen * sizeof(z_ref_data_t));
    self->index = 0;
  }
}

void z_dbuf_reader_get (z_dbuf_reader_t *self, z_dbuf_iovec_t *iovec) {
  iovec->count = 0;
  iovec->offset = self->offset;
  iovec->index = self->index;
  iovec->consumed = self->consumed;
  iovec->node = self->head;
  while (iovec->node != NULL) {
    int node_size = __dbuf_node_data_size(iovec->node);
    while (iovec->offset < node_size && iovec->node->data[iovec->offset] != Z_DBUF_EOF) {
      if (iovec->count == Z_DBUF_NIOVS)
        return;

      switch (iovec->node->data[iovec->offset]) {
        case Z_DBUF_DATA:
          __dbuf_data_get(iovec);
          break;
        case Z_DBUF_REF:
          __dbuf_ref_get(iovec);
          break;
        case Z_DBUF_REF_VEC:
          __dbuf_ref_vec_get(iovec);
          break;
      }
    }
    iovec->offset = 0;
    iovec->index = 0;
    iovec->consumed = 0;
    iovec->node = iovec->node->next;
  }
}

static void __dbuf_reader_remove_head (z_dbuf_reader_t *self) {
  z_dbuf_node_t *node = self->head;

  self->head = node->next;

  z_memory_free(self->memory, z_dbuf_node_t, node, __dbuf_node_length(node));
}

void z_dbuf_reader_remove (z_dbuf_reader_t *self,
                           z_dbuf_iovec_t *iovec,
                           ssize_t rdsize)
{
  int i;
  for (i = 0; i < iovec->count; ++i) {
    if (self->head != iovec->nod[i]) {
      __dbuf_reader_remove_head(self);
    }

    if (rdsize < iovec->iov[i].iov_len) {
      self->offset = iovec->off[i];
      self->index = iovec->idx[i];
      self->consumed += rdsize;
      self->head = iovec->nod[i];
      return;
    }

    if (iovec->vrf[i] != NULL) {
      z_dbuf_vref_t *vref = iovec->vrf[i];
      vref->vtable->dec_ref(vref->udata, iovec->ref[i]->data);
    }

    self->consumed = 0;
    rdsize -= iovec->iov[i].iov_len;
  }

  if (self->head != iovec->node) {
    __dbuf_reader_remove_head(self);
  }
  self->offset = iovec->offset;
  self->index = iovec->index;
  self->head = iovec->node;
}

void z_dbuf_reader_clear (z_dbuf_reader_t *self) {
  z_dbuf_iovec_t iovec;
  /* TODO: Optimize and remove the iovec stuff */
  iovec.count = 0;
  iovec.offset = self->offset;
  iovec.index = self->index;
  iovec.consumed = self->consumed;
  iovec.node = self->head;
  while (iovec.node != NULL) {
    int node_size = __dbuf_node_data_size(iovec.node);
    while (iovec.offset < node_size && iovec.node->data[iovec.offset] != Z_DBUF_EOF) {
      int i;

      iovec.count = 0;
      switch (iovec.node->data[iovec.offset]) {
        case Z_DBUF_DATA:
          __dbuf_data_get(&iovec);
          break;
        case Z_DBUF_REF:
          __dbuf_ref_get(&iovec);
          break;
        case Z_DBUF_REF_VEC:
          __dbuf_ref_vec_get(&iovec);
          break;
      }

      for (i = 0; i < iovec.count; ++i) {
        z_dbuf_vref_t *vref = iovec.vrf[i];
        if (vref == NULL) continue;
        vref->vtable->dec_ref(vref->udata, iovec.ref[i]->data);
      }
    }
    iovec.offset = 0;
    iovec.index = 0;
    iovec.consumed = 0;
    iovec.node = iovec.node->next;
    __dbuf_reader_remove_head(self);
  }
}