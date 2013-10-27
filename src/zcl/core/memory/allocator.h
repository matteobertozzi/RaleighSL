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

#ifndef _Z_ALLOCATOR_H_
#define _Z_ALLOCATOR_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/opaque.h>
#include <zcl/type.h>

#define Z_IMPLEMENT_ALLOCATOR             Z_IMPLEMENT(allocator)
#define Z_ALLOCATOR(x)                    Z_CAST(z_allocator_t, x)

Z_TYPEDEF_STRUCT(z_allocator_interfaces)
Z_TYPEDEF_STRUCT(z_vtable_allocator)
Z_TYPEDEF_STRUCT(z_allocator)

struct z_vtable_allocator {
  void *  (*alloc)    (z_allocator_t *self, size_t size);
  void *  (*realloc)  (z_allocator_t *self, void *ptr, size_t size);
  void    (*free)     (z_allocator_t *self, void *ptr);
};

struct z_allocator_interfaces {
  Z_IMPLEMENT_TYPE
  Z_IMPLEMENT_ALLOCATOR
};

struct z_allocator {
  const z_allocator_interfaces_t *vtables;
  z_opaque_t data;
};

int z_system_allocator_open (z_allocator_t *allocator);

#define z_allocator_close(mem)                                                \
  z_vtable_call(mem, type, dtor)

#define z_allocator_raw_alloc(mem, size)                                      \
  z_vtable_call(mem, allocator, alloc, size)

#define z_allocator_raw_realloc(mem, ptr, size)                               \
  z_vtable_call(mem, allocator, realloc, ptr, size)

#define z_allocator_alloc(mem, type, size)                                    \
  ((type *)z_allocator_raw_alloc(mem, size))

#define z_allocator_realloc(mem, type, size)                                  \
  ((type *)z_allocator_raw_realloc(mem, ptr, size))

#define z_allocator_free(mem, ptr)                                            \
  z_vtable_call(mem, allocator, free, ptr)

__Z_END_DECLS__

#endif /* _Z_ALLOCATOR_H_ */
