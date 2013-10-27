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
#include <zcl/slice.h>

#include <stdio.h>

/* ===========================================================================
 *  PRIVATE Slice Macros
 */
#define __slice_node_blocks_scan(node, block, __ucode__)                    \
  do {                                                                      \
    unsigned int __p_index = Z_SLICE_NBLOCKS;                               \
    block = (node)->blocks;                                                 \
    while (__p_index--) {                                                   \
      do __ucode__ while (0);                                               \
      block++;                                                              \
    }                                                                       \
  } while (0)

/* ===========================================================================
 *  PRIVATE Slice Utils
 */
static void __slice_node_init (z_slice_node_t *node) {
#if 0
  z_byte_slice_t *bslice;
  node->next = NULL;
  __slice_node_blocks_scan(node, bslice, {
    z_byte_slice_clear(bslice);
  });
#else
  z_memzero(node, sizeof(z_slice_node_t));
#endif
}

static z_slice_node_t *__slice_node_alloc (z_slice_t *slice) {
  z_slice_node_t *node;

  node = z_memory_struct_alloc(z_global_memory(), z_slice_node_t);
  if (Z_MALLOC_IS_NULL(node))
    return(NULL);

  __slice_node_init(node);
  return(node);
}

static void __slice_node_free (z_slice_t *slice, z_slice_node_t *node) {
  z_memory_struct_free(z_global_memory(), z_slice_node_t, node);
}

static z_byte_slice_t *__slice_pick_tail_free_buffer (z_slice_node_t *node) {
  unsigned int index = Z_SLICE_NBLOCKS;
  z_byte_slice_t *bslice = NULL;
  while (index--) {
    if (!z_byte_slice_is_empty(&(node->blocks[index])))
      break;
    bslice = &(node->blocks[index]);
  }
  return(bslice);
}

static z_byte_slice_t *__slice_pick_head_free_buffer (z_slice_node_t *node) {
  z_byte_slice_t *bslice;
  __slice_node_blocks_scan(node, bslice, {
    if (z_byte_slice_is_empty(bslice))
      return(bslice);
  });
  return(NULL);
}

/* ===========================================================================
 *  PUBLIC Slice methods
 */
void z_slice_clear (z_slice_t *self) {
  z_slice_node_t *node;
  z_slice_node_t *next;

  for (node = self->head.next; node != NULL; node = next) {
    next = node->next;
    __slice_node_free(self, node);
  }

  self->tail = &(self->head);
  __slice_node_init(self->tail);
}

int z_slice_add (z_slice_t *self, const z_slice_t *slice) {
  const z_byte_slice_t *bslice;
  const z_slice_node_t *node;

  for (node = &(self->head); node != NULL; node = node->next) {
    __slice_node_blocks_scan(node, bslice, {
      if (bslice->length != 0) {
        if (z_slice_append(self, bslice->data, bslice->length))
          return(1);
      }
    });
  }  

  return(0);
}

int z_slice_append (z_slice_t *self, const void *data, size_t size) {
  z_byte_slice_t *bslice;

  if ((bslice = __slice_pick_tail_free_buffer(self->tail)) == NULL) {
    z_slice_node_t *node;

    node = __slice_node_alloc(self);
    if (Z_MALLOC_IS_NULL(node))
      return(-1);

    self->tail->next = node;
    self->tail = node;
    bslice = &(node->blocks[0]);
  }

  z_byte_slice_set(bslice, data, size);
  return(0);
}

int z_slice_prepend (z_slice_t *self, const void *data, size_t size) {
  z_byte_slice_t *bslice = &(self->head.blocks[0]);
  if (!z_byte_slice_is_empty(bslice)) {
    bslice = __slice_pick_head_free_buffer(&(self->head));
    if (bslice != NULL) {
      z_memmove(&(self->head.blocks[1]), &(self->head.blocks[0]), bslice - &(self->head.blocks[0]));
    } else {
      z_slice_node_t *node;

      node = __slice_node_alloc(self);
      if (Z_MALLOC_IS_NULL(node))
        return(-1);

      z_memcpy(node->blocks, self->head.blocks, Z_SLICE_NBLOCKS * sizeof(z_byte_slice_t));
      z_memzero(self->head.blocks, Z_SLICE_NBLOCKS * sizeof(z_byte_slice_t));

      node->next = self->head.next;
      self->head.next = node;
      if (self->tail == &(self->head))
        self->tail = node;

      bslice = &(self->head.blocks[0]);
    }
  }

  z_byte_slice_set(bslice, data, size);
  return(0);
}

int z_slice_ltrim (z_slice_t *self, size_t size) {
  z_byte_slice_t *bslice;
  z_slice_node_t *node;

  for (node = &(self->head); node != NULL; node = node->next) {
    __slice_node_blocks_scan(node, bslice, {
      size_t n = z_min(size, bslice->length);
      size -= n;

      bslice->data += n;
      bslice->length -= n;
    });
  }

  return(0);
}

size_t z_slice_length (const z_slice_t *self) {
  const z_byte_slice_t *bslice;
  const z_slice_node_t *node;
  size_t length = 0;

  for (node = &(self->head); node != NULL; node = node->next) {
    __slice_node_blocks_scan(node, bslice, {
      if (bslice->length == 0)
        return(length);
      length += bslice->length;
    });
  }

  return(length);
}

int z_slice_is_empty (const z_slice_t *self) {
  return(z_byte_slice_is_empty(&(self->head.blocks[0])));
}

int z_slice_equals (const z_slice_t *self, const void *data, size_t size) {
  const uint8_t *pdata = (const uint8_t *)data;
  const z_byte_slice_t *bslice;
  const z_slice_node_t *node;

  for (node = &(self->head); node != NULL; node = node->next) {
    __slice_node_blocks_scan(node, bslice, {
      size_t n;

      if (size < bslice->length)
        return(0);

      if ((n = z_min(size, bslice->length)) == 0)
        return(size == 0);

      if (!z_byte_slice_starts_with(bslice, pdata, n))
        return(0);

      pdata += n;
      size -= n;
    });
  }

  return(size == 0);
}

int z_slice_starts_with (const z_slice_t *self, const void *data, size_t size) {
  const uint8_t *pdata = (const uint8_t *)data;
  const z_byte_slice_t *bslice;
  const z_slice_node_t *node;

  for (node = &(self->head); node != NULL; node = node->next) {
    __slice_node_blocks_scan(node, bslice, {
      size_t n;

      if ((n = z_min(size, bslice->length)) == 0)
        return(size == 0);

      if (!z_byte_slice_starts_with(bslice, pdata, size))
        return(0);

      pdata += n;
      size -= n;
    });
  }

  return(size == 0);
}

size_t z_slice_dump (FILE *stream, const z_slice_t *self) {
  const z_byte_slice_t *bslice;
  const z_slice_node_t *node;
  size_t length = 0;
  int i, j;

  fprintf(stream, "%zu:", z_slice_length(self));
  for (node = &(self->head); node != NULL; node = node->next) {
    for (i = 0; i < Z_SLICE_NBLOCKS; ++i) {
      bslice = &(node->blocks[i]);
      if (bslice->length == 0) {
        fputc('\n', stream);
        return(length);
      }

      length += bslice->length;
      for (j = 0; j < bslice->length; ++j) {
        fputc(bslice->data[j], stream);
      }
    }
  }
  fputc('\n', stream);
  return(length);
}

size_t z_slice_copy (const z_slice_t *self, void *buf, size_t size) {
  uint8_t *pbuf = (uint8_t *)buf;
  const z_byte_slice_t *bslice;
  const z_slice_node_t *node;

  for (node = &(self->head); node != NULL; node = node->next) {
    __slice_node_blocks_scan(node, bslice, {
      size_t n = z_min(size, bslice->length);
      size -= n;

      z_memcpy(pbuf, bslice->data, n);
      pbuf += n;
    });
  }

  *pbuf = '\0';
  return(pbuf - (uint8_t *)buf);
}

/* ===========================================================================
 *  INTERFACE Type methods
 */
static int __slice_ctor (void *self, va_list args) {
  z_slice_t *slice = Z_SLICE(self);
  slice->tail = &(slice->head);
  __slice_node_init(slice->tail);
  return(0);
}

static void __slice_dtor (void *self) {
  z_slice_clear(Z_SLICE(self));
}

/* ===========================================================================
 *  INTERFACE Reader methods
 */
static int __slice_reader_open (void *self, const void *object) {
  z_slice_reader_t *reader = Z_SLICE_READER(self);
  const z_slice_t *slice = Z_CONST_SLICE(object);
  Z_READER_INIT(reader, slice);
  reader->node = &(slice->head);
  reader->iblock = 0;
  reader->offset = 0;
  return(0);
}

static void __slice_reader_close (void *self) {
}

static size_t __slice_reader_next (void *self, const uint8_t **data) {
  z_slice_reader_t *reader = Z_SLICE_READER(self);
  const z_slice_node_t *node = reader->node;
  const z_byte_slice_t *block = &(node->blocks[reader->iblock]);
  size_t n;

  if (node == NULL || z_byte_slice_is_empty(block))
    return(0);

  /* still some bytes left */
  if (reader->offset < block->length) {
    *data = block->data + reader->offset;
    n = block->length - reader->offset;
    reader->offset = block->length;
    return(n);
  }

  if (reader->iblock++ < Z_SLICE_NBLOCKS) {
    block = &(node->blocks[reader->iblock]);
  } else {
    if ((node = node->next) == NULL)
      return(0);

    reader->node = node;
    reader->iblock = 0;
    block = &(node->blocks[0]);
  }

  if (z_byte_slice_is_empty(block))
    return(0);

  reader->offset = block->length;
  *data = block->data;
  return(block->length);
}

static void __slice_reader_backup (void *self, size_t count) {
  z_slice_reader_t *reader = Z_SLICE_READER(self);
  reader->offset -= count;
}

static size_t __slice_reader_available (void *self) {
  z_slice_reader_t *reader = Z_SLICE_READER(self);
  const z_slice_t *slice = Z_READER_READABLE(z_slice_t, self);
  return(z_slice_length(slice) - reader->offset);
}

/* ===========================================================================
 *  Slice vtables
 */
static const z_vtable_type_t __slice_type = {
  .name = "Slice",
  .size = sizeof(z_slice_t),
  .ctor = __slice_ctor,
  .dtor = __slice_dtor,
};

static const z_vtable_reader_t __slice_reader = {
  .open       = __slice_reader_open,
  .close      = __slice_reader_close,
  .next       = __slice_reader_next,
  .backup     = __slice_reader_backup,
  .available  = __slice_reader_available,
  .fetch      = NULL,
};

static const z_slice_interfaces_t __slice_interfaces = {
  .type   = &__slice_type,
  .reader = &__slice_reader,
};

/* ===========================================================================
*  PUBLIC Slice constructor/destructor
*/
z_slice_t *z_slice_alloc (z_slice_t *self) {
  return(z_object_alloc(self, &__slice_interfaces));
}

void z_slice_free (z_slice_t *self) {
  z_object_free(self);
}
