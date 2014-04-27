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

#ifndef _Z_DATA_MEMREF_H_
#define _Z_DATA_MEMREF_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <string.h>

#include <zcl/macros.h>
#include <zcl/memslice.h>

#define Z_CONST_BYTES_REF(x)              Z_CONST_CAST(z_memref_t, x)
#define Z_BYTES_REF(x)                    Z_CAST(z_memref_t, x)

Z_TYPEDEF_STRUCT(z_vtable_refs)
Z_TYPEDEF_STRUCT(z_memref)

struct z_vtable_refs {
  void (*inc_ref) (void *object);
  void (*dec_ref) (void *object);
};

struct z_memref {
  z_memslice_t slice;
  const z_vtable_refs_t *vtable;
  void *    object;
};

#define z_memref_slice(self)         (&((self)->slice))
#define z_memref_data(self)          ((self)->slice.data)
#define z_memref_size(self)          ((self)->slice.size)
#define z_memref_is_empty(self)      z_memslice_is_empty(&((self)->slice))

#define z_memref_compare(a, b)                                       \
  z_memslice_compare(z_memref_slice(a), z_memref_slice(b))

void z_memref_reset    (z_memref_t *self);
void z_memref_set      (z_memref_t *self,
                           const z_memslice_t *slice,
                           const z_vtable_refs_t *vtable,
                           void *object);
void z_memref_set_data (z_memref_t *self,
                           const void *data,
                           unsigned int size,
                           const z_vtable_refs_t *vtable,
                           void *object);
void z_memref_acquire  (z_memref_t *self,
                           const z_memref_t *other);
void z_memref_release  (z_memref_t *self);

__Z_END_DECLS__

#endif /* _Z_DATA_MEMREF_H_ */
