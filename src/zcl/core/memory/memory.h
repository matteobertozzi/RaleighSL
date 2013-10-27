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

#ifndef _Z_MEMORY_H_
#define _Z_MEMORY_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/mempool.h>
#include <zcl/allocator.h>

#define Z_MEMORY_ALLOCATOR(x)         (Z_MEMORY(x)->allocator)
#define Z_MEMORY_DATA(x)              (Z_MEMORY(x)->data)
#define Z_MEMORY(x)                   Z_CAST(z_memory_t, x)

#define Z_MALLOC_IS_NULL(x)           (Z_UNLIKELY((x) == NULL))

Z_TYPEDEF_STRUCT(z_memory)

/* 16, 32, 48, 64, 80, 96 */
/* 16, 24, 32, 40, 48, 56, 64, 72 */
#define Z_MEMORY_POOL_ALIGN_SHIFT      (4)
#define Z_MEMORY_POOL_MAX_ISIZE        (96)
#define Z_MEMORY_POOL_SIZE             (Z_MEMORY_POOL_MAX_ISIZE >> Z_MEMORY_POOL_ALIGN_SHIFT)

#define z_memory_pool_align(x)         z_align_up(x, 1 << Z_MEMORY_POOL_ALIGN_SHIFT)
#define z_memory_pool_index(x)         ((z_memory_pool_align(x) >> Z_MEMORY_POOL_ALIGN_SHIFT) - 1)

struct z_memory {
  z_allocator_t *allocator;
  z_mempool_t pools[Z_MEMORY_POOL_SIZE];
};

typedef void (*z_mem_free_t) (void *udata, void *object);

z_memory_t *z_memory_open   (z_memory_t *memory, z_allocator_t *allocator);
void        z_memory_close  (z_memory_t *memory);

/* basic alloc/realloc/free */
#define z_memory_raw_alloc(memory, size)                                      \
  z_allocator_raw_alloc(memory->allocator, size)

#define z_memory_raw_realloc(memory, ptr, size)                               \
  z_allocator_raw_realloc(memory->allocator, ptr, size)

#define z_memory_alloc(memory, type, size)                                    \
  ((type *)z_memory_raw_alloc(memory, size))

#define z_memory_realloc(memory, type, ptr, size)                             \
  ((type *)z_memory_raw_realloc(memory, ptr, size))

#define z_memory_free(memory, ptr)                                            \
  z_allocator_free(memory->allocator, ptr)

/* struct alloc/free */
#define z_memory_struct_alloc(memory, type)                                   \
  ((type *)__z_memory_struct_alloc(memory, sizeof(type)))

#define z_memory_struct_free(memory, type, ptr)                               \
  __z_memory_struct_free(memory, sizeof(type), ptr)

/* array alloc/realloc/free */
#define z_memory_array_alloc(memory, type, n)                                 \
  z_memory_alloc(memory, type, (n) * sizeof(type))

#define z_memory_array_realloc(memory, ptr, type, n)                          \
  z_memory_realloc(memory, type, ptr, (n) * sizeof(type))

#define z_memory_array_free(memory, ptr)                                      \
  z_memory_free(memory, ptr)

/* utils */
#define z_memory_dup(memory, type, src, size)                                 \
  ((type *)__z_memory_dup(memory, src, size))

void *__z_memory_struct_alloc (z_memory_t *memory, size_t size);
void  __z_memory_struct_free  (z_memory_t *memory, size_t size, void *ptr);

void *__z_memory_dup    (z_memory_t *memory, const void *src, size_t size);

__Z_END_DECLS__

#endif /* _Z_MEMORY_H_ */
