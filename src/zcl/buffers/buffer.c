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
#include <zcl/string.h>
#include <zcl/buffer.h>

#define __buffer_blksize(self)            (Z_OBJECT(self)->flags.u32)
#define __buffer_set_blksize(self, n)     Z_OBJECT(self)->flags.u32 = n;

/* ===========================================================================
 *  PRIVATE Byte Array utils
 */
static int __buffer_grow (z_buffer_t *self, size_t size) {
  uint8_t *block;

#if 0
  size = self->grow_policy(self->user_data, size);
#else
  size += (__buffer_blksize(self) > 0) ? ((size < 1024) ? 32 : 1024) : 0;
#endif

  //fprintf(stderr, "Buffer Realloc %u -> %zu\n", __buffer_blksize(self), size);
  size = z_align_up(size, 32);
  block = z_memory_realloc(z_global_memory(), uint8_t, self->block, size);
  if (Z_MALLOC_IS_NULL(block))
    return(-1);

  __buffer_set_blksize(self, size);
  self->block = block;
  return(0);
}

/* ===========================================================================
 *  PUBLIC Byte Array
 */
void z_buffer_release (z_buffer_t *self) {
  __buffer_set_blksize(self, 0);
  self->block = NULL;
  self->size = 0U;
}

int z_buffer_squeeze (z_buffer_t *self) {
  uint8_t *block;
  size_t size;

  if (self->size > 0) {
    size = self->size;
  } else {
#if 0
    size = self->grow_policy(self->user_data, 0);
#else
    size = 32;
#endif
  }

  block = z_memory_realloc(z_global_memory(), uint8_t, self->block, size);
  if (Z_MALLOC_IS_NULL(block))
    return(-1);

  __buffer_set_blksize(self, size);
  self->block = block;
  return(0);
}

int z_buffer_reserve (z_buffer_t *self, size_t reserve) {
  if (reserve > __buffer_blksize(self)) {
    return(__buffer_grow(self, reserve));
  }
  return(0);
}

int z_buffer_clear (z_buffer_t *self) {
  self->size = 0U;
  return(0);
}

int z_buffer_truncate (z_buffer_t *self, size_t size) {
  if (Z_UNLIKELY(size >= self->size))
    return(-1);

  self->size = size;
  return(0);
}

int z_buffer_remove (z_buffer_t *self, size_t index, size_t size) {
  if (Z_UNLIKELY(index >= self->size || size == 0))
    return(-1);

  if ((index + size) >= self->size) {
    self->size = index;
  } else {
    uint8_t *p;

    p = self->block + index;
    z_memmove(p, p + size, self->size - index - size);
    self->size -= size;
  }

  return(0);
}

int z_buffer_set (z_buffer_t *self, const void *blob, size_t size) {
  if (size > __buffer_blksize(self)) {
    if (Z_UNLIKELY(__buffer_grow(self, size)))
      return(-1);
  }

  z_memmove(self->block, blob, size);
  self->size = size;
  return(0);
}

int z_buffer_append (z_buffer_t *self, const void *blob, size_t size) {
  size_t n;

  if (Z_UNLIKELY((n = (self->size + size)) >= __buffer_blksize(self))) {
    if (Z_UNLIKELY(__buffer_grow(self, n)))
      return(-1);
  }

  z_memcpy(self->block + self->size, blob, size);
  self->size = n;
  return(0);
}

int z_buffer_prepend (z_buffer_t *self, const void *blob, size_t size) {
  size_t n;

  if ((n = (self->size + size)) > __buffer_blksize(self)) {
    if (__buffer_grow(self, n))
      return(-1);
  }

  if ((uint8_t *)blob >= self->block &&
      (uint8_t *)blob < (self->block + __buffer_blksize(self)))
  {
    z_memory_t *memory = z_global_memory();
    uint8_t *dblob;

    if ((dblob = z_memory_dup(memory, uint8_t, blob, size)) == NULL)
      return(-1);

    z_memmove(self->block + size, self->block, self->size);
    z_memcpy(self->block, dblob, size);

    z_memory_free(memory, dblob);
  } else {
    z_memmove(self->block + size, self->block, self->size);
    z_memcpy(self->block, blob, size);
  }

  self->size += size;
  return(0);
}

int z_buffer_insert (z_buffer_t *self, size_t index, const void *blob, size_t n) {
  size_t size;
  uint8_t *p;

  size = self->size + n;
  if (index > self->size)
    size += (index - self->size);

  if (size > __buffer_blksize(self)) {
    if (__buffer_grow(self, size))
      return(-1);
  }

  p = (self->block + index);
  if (index > self->size) {
    z_memzero(self->block + self->size, index - self->size);
    z_memcpy(p, blob, n);
  } else if ((uint8_t *)blob >= self->block &&
             (uint8_t *)blob < (self->block + __buffer_blksize(self)))
  {
    z_memory_t *memory = z_global_memory();
    uint8_t *dblob;

    if ((dblob = z_memory_dup(memory, uint8_t, blob, n)) == NULL)
      return(-1);

    z_memmove(p + n, p, self->size - index);
    z_memcpy(p, dblob, n);

    z_memory_free(memory, dblob);
  } else {
    z_memmove(p + n, p, self->size - index);
    z_memcpy(p, blob, n);
  }

  self->size = size;
  return(0);
}

int z_buffer_replace (z_buffer_t *self,
                     size_t index,
                     size_t size,
                     const void *blob,
                     size_t n)
{
  if (size == n && (index + n) <= self->size) {
    z_memmove(self->block + index, blob, n);
  } else {
    z_buffer_remove(self, index, size);
    if (z_buffer_insert(self, index, blob, n))
      return(-3);
  }
  return(0);
}

int z_buffer_equal (z_buffer_t *self, const void *blob, size_t size) {
  if (self->size != size)
    return(0);
  return(!z_memcmp(self->block, blob, size));
}

int z_buffer_compare (z_buffer_t *self, const void *blob, size_t size) {
  if (self->size < size)
    size = self->size;
  return(z_memcmp(self->block, blob, size));
}


/* ===========================================================================
 *  INTERFACE Type methods
 */
static int __buffer_ctor (void *self, va_list args) {
  z_buffer_t *buffer = Z_BUFFER(self);
  __buffer_set_blksize(buffer, 0);
  buffer->block = NULL;
  buffer->size = 0U;
  return(0);
}

static void __buffer_dtor (void *self) {
  z_buffer_t *buffer = Z_BUFFER(self);

  if (buffer->block != NULL) {
    z_memory_free(z_global_memory(), buffer->block);
    buffer->block = NULL;
  }

  __buffer_set_blksize(buffer, 0U);
  buffer->size = 0U;
}

/* ===========================================================================
 *  Bytes vtables
 */
static const z_vtable_type_t __buffer_type = {
  .name = "Buffer",
  .size = sizeof(z_buffer_t),
  .ctor = __buffer_ctor,
  .dtor = __buffer_dtor,
};

static const z_type_interfaces_t __buffer_interfaces = {
  .type = &__buffer_type,
};

/* ===========================================================================
 *  PUBLIC Bytes constructor/destructor
 */
z_buffer_t *z_buffer_alloc (z_buffer_t *self) {
  return(z_object_alloc(self, &__buffer_interfaces));
}

void z_buffer_free (z_buffer_t *self) {
  z_object_free(self);
}
