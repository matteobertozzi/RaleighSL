/*
 *   Copyright 2011-2012 Matteo Bertozzi
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

#include <zcl/type.h>

#define Z_OBJECT_FLAGS_MEM_STACK (1 << 0)
#define Z_OBJECT_FLAGS_MEM_HEAP  (1 << 1)

Z_TYPEDEF_STRUCT(z_object)

struct z_object {
    z_memory_t *memory;
    struct obj_flags {
        uint16_t iflags;        /* Internal flags */
        uint16_t uflags16;      /* User flags 16bit (free extra space) */
        uint32_t uflags32;      /* User flags 32bit (free extra space) */
    } flags;
};

#define __Z_OBJECT__(type)                                              \
    z_object_t __object__;                                              \
    const type ## _interfaces_t *vtables;

#define z_object_alloc(obj, interfaces, memory, ...)                    \
    __z_object_alloc(Z_OBJECT(obj),                                     \
                     ((obj)->vtables = interfaces)->type,               \
                     memory, ##__VA_ARGS__)

#define z_object_free(obj)                                              \
  __z_object_free(Z_OBJECT(obj), z_vtable_type(obj))

#define z_object_flags(obj)         Z_OBJECT(obj)->flags
#define z_object_memory(obj)        Z_OBJECT(obj)->memory

#define z_object_block_alloc(obj, type, size)                           \
  z_memory_alloc(z_object_memory(obj), type, size)

#define z_object_block_free(obj, ptr)                                   \
  z_memory_free(z_object_memory(obj), ptr)

#define z_object_struct_alloc(obj, type)                                \
    z_memory_struct_alloc(z_object_memory(obj), type)

#define z_object_struct_free(obj, type, ptr)                            \
    z_memory_struct_free(z_object_memory(obj), type, ptr)

void *__z_object_alloc  (z_object_t *object,
                         const z_vtable_type_t *type,
                         z_memory_t *memory, ...);
void  __z_object_free   (z_object_t *object,
                         const z_vtable_type_t *type);

__Z_END_DECLS__

#endif /* !_Z_OBJECT_H_ */
