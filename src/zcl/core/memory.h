/*
 *   Copyright 2011 Matteo Bertozzi
 *
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

#include <zcl/allocator.h>
#include <zcl/macros.h>

#define Z_MEMORY_POOL_SHIFT                 (4)
#define Z_MEMORY_POOL_SIZE                  (128 >> Z_MEMORY_POOL_SHIFT)

Z_TYPEDEF_STRUCT(z_mempage)
Z_TYPEDEF_STRUCT(z_memobj)
Z_TYPEDEF_STRUCT(z_mempool)
Z_TYPEDEF_STRUCT(z_memory)

struct z_mempage {
    z_mempage_t *next;              /* Next page with header */
    uint8_t *    blocks[1];         /* List of full pages */
};

struct z_memobj {
    z_memobj_t *next;
    unsigned int used;
};

struct z_mempool {
    z_mempage_t *pages;             /* Page Head */
    z_memobj_t * objects;           /* Object Stack */
};

struct z_memory {
    z_mempool_t pool[Z_MEMORY_POOL_SIZE];
    z_allocator_t * allocator;
};

void    z_memory_init       (z_memory_t *memory,
                             z_allocator_t *allocator);

void *  z_memory_zalloc     (z_memory_t *memory,
                             unsigned int size);

#define z_memory_alloc(memory, size)                                        \
    (memory)->allocator->alloc((memory)->allocator, size)

#define z_memory_realloc(memory, ptr, size)                                 \
    (memory)->allocator->realloc((memory)->allocator, ptr, size)

#define z_memory_free(memory, ptr)                                          \
    (memory)->allocator->free((memory)->allocator, ptr)

#define z_memory_struct_alloc(memory, type)                                 \
    ((type *)z_memory_pool_pop(memory, sizeof(type)))

#define z_memory_struct_zalloc(memory, type)                                \
    ((type *)z_memory_pool_pop(memory, sizeof(type)))

#define z_memory_struct_free(memory, type, ptr)                             \
    z_memory_pool_push(memory, sizeof(type), ptr)

#define z_memory_array_alloc(memory, n, type)                               \
    ((type *)z_memory_alloc(memory, (n) * sizeof(type)))

#define z_memory_array_zalloc(memory, n, type)                              \
    ((type *)z_memory_zalloc(memory, (n) * sizeof(type)))

#define z_memory_array_realloc(memory, ptr, n, type)                        \
    ((type *)z_memory_realloc(memory, ptr, (n) * sizeof(type)))

#define z_memory_array_free(memory, ptr)                                    \
    z_memory_free(memory, ptr)

#define z_memory_block_alloc(memory, type, n)                               \
    ((type *)z_memory_alloc(memory, n))

#define z_memory_block_realloc(memory, type, ptr, n)                        \
    ((type *)z_memory_realloc(memory, ptr, n))

#define z_memory_block_zalloc(memory, type, n)                              \
    ((type *)z_memory_zalloc(memory, n))

#define z_memory_block_free(memory, ptr)                                    \
    z_memory_free(memory, ptr)

#define z_memory_blob_alloc(memory, n)                                      \
    z_memory_block_alloc(memory, uint8_t, n)

#define z_memory_blob_realloc(memory, ptr, n)                               \
    z_memory_block_realloc(memory, uint8_t, ptr, n)

#define z_memory_blob_zalloc(memory, uint8_t, n)                            \
    z_memory_block_zalloc(memory, uint8_t, n)

#define z_memory_blob_free(memory, ptr)                                     \
    z_memory_block_free(memory, ptr)

void *  z_memory_pool_pop   (z_memory_t *memory,
                             unsigned int size);
void *  z_memory_pool_zpop  (z_memory_t *memory,
                             unsigned int size);
void    z_memory_pool_push  (z_memory_t *memory,
                             unsigned int size,
                             void *ptr);

void *  __z_memory_dup      (z_memory_t *memory,
                             const void *src,
                             unsigned int size);
#define z_memory_dup(memory, type, src, size)                               \
    ((type *)__z_memory_dup(memory, src, size))

#endif /* _Z_MEMORY_H_ */

