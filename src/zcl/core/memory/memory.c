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

#include <zcl/memory.h>
#include <zcl/string.h>
#include <zcl/debug.h>

z_memory_t *z_memory_open (z_memory_t *memory, z_allocator_t *allocator) {
  int i;

  memory->allocator = allocator;

  /* Initialize Memory Pool */
  for (i = 0; i < Z_MEMORY_POOL_SIZE; ++i) {
    z_mempool_open(&(memory->pools[i]), allocator, (i + 1) << Z_MEMORY_POOL_ALIGN_SHIFT);
  }

  return(memory);
}

void z_memory_close (z_memory_t *memory) {
  int i;

  /* Uninitialize Memory Pool */
  for (i = 0; i < Z_MEMORY_POOL_SIZE; ++i) {
    z_mempool_close(&(memory->pools[i]), memory->allocator);
  }
}

void *__z_memory_struct_alloc (z_memory_t *memory, size_t size) {
  size_t index = z_memory_pool_index(size);
  if (Z_LIKELY(index < Z_MEMORY_POOL_SIZE)) {
    void *ptr;
    if ((ptr = z_mempool_alloc(&(memory->pools[index]))) != NULL)
      return(ptr);
  }

  return(z_memory_raw_alloc(memory, size));
}

void __z_memory_struct_free (z_memory_t *memory, size_t size, void *ptr) {
  size_t index = z_memory_pool_index(size);
  if (Z_LIKELY(index < Z_MEMORY_POOL_SIZE)) {
    if (!z_mempool_free(&(memory->pools[index]), ptr))
      return;
  }

  z_memory_free(memory, ptr);
}

void *__z_memory_dup (z_memory_t *memory, const void *src, size_t size) {
  void *dst;

  if ((dst = z_memory_raw_alloc(memory, size)) != NULL) {
    z_memcpy(dst, src, size);
  }

  return(dst);
}
