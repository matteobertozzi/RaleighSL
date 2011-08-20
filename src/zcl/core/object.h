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

#ifndef _Z_OBJECT_H_
#define _Z_OBJECT_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/memory.h>
#include <zcl/macros.h>

#define Z_OBJECT_TYPE_ID(obj_t)         obj_t##ype_id
#define Z_OBJECT_TYPE                   z_object_t __base__;
#define Z_OBJECT(x)                     Z_CAST(z_object_t, x)

#define Z_OBJECT_FLAGS_MEM_STACK        (1 << 0)
#define Z_OBJECT_FLAGS_MEM_HEAP         (1 << 1)
#define Z_OBJECT_FLAGS_SHIFT            (8)

Z_TYPEDEF_STRUCT(z_object)

struct z_object {
    z_memory_t *     memory;

    struct _flags {
        uint16_t    type;
        uint8_t     unused;
        uint8_t     base_flags;
        uint32_t    user_flags;
    } flags;
};

#define z_object_alloc(memory, obj, obj_type)                               \
    ((obj_type *)__z_object_alloc(memory, Z_OBJECT(obj),                    \
                                  Z_OBJECT_TYPE_ID(obj_type),               \
                                  sizeof(obj_type)))

#define z_object_free(obj)                                                  \
    __z_object_free(Z_OBJECT(obj))

#define z_object_memory(obj)                                                \
    (Z_OBJECT(obj)->memory)

#define z_object_type(obj)                                                  \
    (Z_OBJECT(obj)->flags.type)

#define z_object_is_instance(obj, type)                                     \
    ((z_object_type(obj) & Z_OBJECT_TYPE_ID(type)) == Z_OBJECT_TYPE_ID(type))

#define z_object_flags(obj)                                                 \
    (Z_OBJECT(obj)->flags.user_flags)

#define z_object_struct_alloc(object, type)                                 \
    z_memory_struct_alloc(z_object_memory(object), type)

#define z_object_struct_zalloc(object, type)                                \
    z_memory_struct_zalloc(z_object_memory(object), type)

#define z_object_struct_free(object, type, ptr)                             \
    z_memory_struct_free(z_object_memory(object), type, ptr)

#define z_object_array_alloc(object, n, type)                               \
    z_memory_array_alloc(z_object_memory(object), n, type)

#define z_object_array_zalloc(object, n, type)                              \
    z_memory_array_zalloc(z_object_memory(object), n, type)

#define z_object_array_realloc(object, ptr, n, type)                        \
    z_memory_array_realloc(z_object_memory(object), ptr, n, type)

#define z_object_array_free(object, ptr)                                    \
    z_memory_array_free(z_object_memory(object), ptr)

#define z_object_block_alloc(object, type, n)                               \
    z_memory_block_alloc(z_object_memory(object), type, n)

#define z_object_block_realloc(object, type, ptr, n)                        \
    z_memory_block_realloc(z_object_memory(object), type, ptr, n)

#define z_object_block_zalloc(object, type, n)                              \
    z_memory_block_zalloc(z_object_memory(object), type, n)

#define z_object_block_free(object, ptr)                                    \
    z_memory_block_free(z_object_memory(object), ptr)

#define z_object_blob_alloc(object, n)                                      \
    z_memory_blob_alloc(z_object_memory(object), n)

#define z_object_blob_realloc(object, ptr, n)                               \
    z_memory_blob_realloc(z_object_memory(object), ptr, n)

#define z_object_blob_zalloc(object, n)                                     \
    z_memory_blob_zalloc(z_object_memory(object), n)

#define z_object_blob_free(object, ptr)                                    \
    z_memory_blob_free(z_object_memory(object), ptr)

void *  __z_object_alloc    (z_memory_t *memory,
                             z_object_t *object,
                             unsigned short int type,
                             unsigned int size);
void    __z_object_free     (z_object_t *object);

__Z_END_DECLS__

#endif /* !_Z_OBJECT_H_ */

