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

#ifndef _Z_MEMORY_ALLOCATOR_H_
#define _Z_MEMORY_ALLOCATOR_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

Z_TYPEDEF_STRUCT(z_vtable_allocator)
Z_TYPEDEF_STRUCT(z_allocator)

struct z_vtable_allocator {
  void *  (*alloc)    (z_allocator_t *self, size_t size);
  void *  (*realloc)  (z_allocator_t *self, void *ptr, size_t size);
  void    (*free)     (z_allocator_t *self, void *ptr);

  int     (*ctor)     (z_allocator_t *self);
  void    (*dtor)     (z_allocator_t *self);
};

struct z_allocator {
  const z_vtable_allocator_t *vtable;
  void *data;
};

int z_system_allocator_open (z_allocator_t *allocator);

#define z_allocator_close(mem)                                                \
  (mem)->vtable->dtor(mem)

#define z_allocator_raw_alloc(mem, size)                                      \
  (mem)->vtable->alloc(mem, size)

#define z_allocator_raw_realloc(mem, ptr, size)                               \
  (mem)->vtable->realloc(mem, ptr, size)

#define z_allocator_alloc(mem, type, size)                                    \
  ((type *)z_allocator_raw_alloc(mem, size))

#define z_allocator_realloc(mem, type, size)                                  \
  ((type *)z_allocator_raw_realloc(mem, ptr, size))

#define z_allocator_free(mem, ptr)                                            \
  (mem)->vtable->free(mem, ptr)

__Z_END_DECLS__

#endif /* _Z_MEMORY_ALLOCATOR_H_ */
