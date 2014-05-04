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
#include <zcl/array.h>
#include <zcl/debug.h>
#include <zcl/mem.h>

static z_array_block_t *__array_block_alloc(z_array_t *self) {
  z_array_block_t *block;

  block = z_memory_struct_alloc(z_global_memory(), z_array_block_t);
  if (Z_MALLOC_IS_NULL(block))
    return(NULL);

  z_memzero(block, sizeof(z_array_block_t));
  return(block);
}

int z_array_open (z_array_t *self, int type_size) {
  self->head.next = NULL;
  self->tail = &(self->head);
  self->count = 0;
  self->type_size = type_size;
  self->per_block = Z_ARRAY_BLOCK_SIZE / type_size;
  return(0);
}

void z_array_close (z_array_t *self) {
  z_array_clear(self);
}

void z_array_clear (z_array_t *self) {
  z_memory_t *memory = z_global_memory();
  z_array_block_t *block = self->head.next;
  self->head.next = NULL;
  self->count = 0;
  while (block != NULL) {
    z_array_block_t *next = block->next;
    z_memory_struct_free(memory, z_array_block_t, block);
    block = next;
  }
  memory = NULL;
}

const void *z_array_get_raw (const z_array_t *self, size_t index) {
  const z_array_block_t *block = &(self->head);
  while (index >= self->per_block) {
    index -= self->per_block;
    block = block->next;
  }
  return(block->items.data + (index * self->type_size));
}

const void *z_array_get_raw_ptr (const z_array_t *self, size_t index) {
  const z_array_block_t *block = &(self->head);
  while (index >= self->per_block) {
    index -= self->per_block;
    block = block->next;
  }
  return(block->items.ptrs[index]);
}

void *z_array_push_back (z_array_t *self) {
  z_array_block_t *block = self->tail;
  void *dst;
  int index;

  index = self->count % self->per_block;
  if (index == 0 && self->count > 0) {
    block->next = __array_block_alloc(self);
    if (Z_MALLOC_IS_NULL(block->next))
      return(NULL);

    block = block->next;
    self->tail = block;
    dst = block->items.data;
  } else {
    dst = block->items.data + (index * self->type_size);
  }

  self->count++;
  return(dst);
}

int z_array_push_back_copy (z_array_t *self, const void *data) {
  void *dst;

  dst = z_array_push_back(self);
  if (Z_MALLOC_IS_NULL(dst))
    return(1);

  z_memcpy(dst, data, self->type_size);
  return(0);
}

int z_array_push_back_ptr (z_array_t *self, const void *ptr) {
  z_array_block_t *block = self->tail;
  int index;

  index = self->count % self->per_block;
  if (index == 0 && self->count > 0) {
    block->next = __array_block_alloc(self);
    if (Z_MALLOC_IS_NULL(block->next))
      return(1);

    block = block->next;
    self->tail = block;
  }

  block->items.ptrs[index] = ptr;
  self->count++;
  return(0);
}
