/*
 *   Copyright 2011-2013 Matteo Bertozzi
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

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/allocator.h>
#include <zcl/macros.h>

#define Z_MEMORY_ALLOCATOR(x)   (Z_MEMORY(x)->allocator)
#define Z_MEMORY_DATA(x)        (Z_MEMORY(x)->data)
#define Z_MEMORY(x)             Z_CAST(z_memory_t, x)

Z_TYPEDEF_STRUCT(z_memory)

struct z_memory {
    const z_allocator_t *allocator;
    void *data;
};

z_memory_t *z_memory_init   (z_memory_t *memory,
                             const z_allocator_t *allocator,
                             void *user_data);
void *      __z_memory_dup  (z_memory_t *memory,
                             const void *src,
                             unsigned int size);
void *      __z_memory_zero (void *data,
                             unsigned int size);

#define z_system_memory_init(memory)                                          \
    z_memory_init(memory, z_system_allocator(), NULL)

/* basic alloc/realloc/free */
#define z_memory_raw_alloc(memory, size)                                      \
    Z_MEMORY_ALLOCATOR(memory)->alloc(Z_MEMORY_DATA(memory), size)

#define z_memory_alloc(memory, type, size)                                    \
    ((type *)Z_MEMORY_ALLOCATOR(memory)->alloc(Z_MEMORY_DATA(memory), size))

#define z_memory_zalloc(memory, type, size)                                   \
    ((type *)__z_memory_zero(z_memory_alloc(memory, type, size), size))

#define z_memory_realloc(memory, type, ptr, size)                             \
    ((type *)(Z_MEMORY_ALLOCATOR(memory)->realloc(Z_MEMORY_DATA(memory), ptr, size)))

#define z_memory_free(memory, ptr)                                            \
    Z_MEMORY_ALLOCATOR(memory)->free(Z_MEMORY_DATA(memory), ptr)

/* struct alloc/free */
#define z_memory_struct_alloc(memory, type)                                   \
    z_memory_alloc(memory, type, sizeof(type))

#define z_memory_struct_free(memory, type, ptr)                               \
    z_memory_free(memory, ptr)

/* array alloc/realloc/free */
#define z_memory_array_alloc(memory, type, n)                                 \
    z_memory_alloc(memory, type, (n) * sizeof(type))

#define z_memory_array_zalloc(memory, type, n)                                \
    z_memory_zalloc(memory, type, (n) * sizeof(type))

#define z_memory_array_realloc(memory, ptr, type, n)                          \
    z_memory_realloc(memory, type, ptr, (n) * sizeof(type))

#define z_memory_array_free(memory, ptr)                                      \
    z_memory_free(memory, ptr)

/* blob alloc/realloc/free */
#define z_memory_blob_alloc(memory, n)                                        \
    z_memory_alloc(memory, unsigned char, n)

#define z_memory_blob_realloc(memory, ptr, n)                                 \
    z_memory_realloc(memory, unsigned char, ptr, n)

#define z_memory_blob_free(memory, ptr)                                       \
    z_memory_free(memory, ptr)

#define z_memory_dup(memory, type, src, size)                                 \
    ((type *)__z_memory_dup(memory, src, size))

__Z_END_DECLS__

#endif /* !_Z_MEMORY_H_ */
