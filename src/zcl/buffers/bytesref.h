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

#ifndef _Z_BYTES_REFS_H_
#define _Z_BYTES_REFS_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/byteslice.h>
#include <zcl/object.h>
#include <zcl/macros.h>

#define Z_CONST_BYTES_REF(x)              Z_CONST_CAST(z_bytes_ref_t, x)
#define Z_BYTES_REF(x)                    Z_CAST(z_bytes_ref_t, x)

Z_TYPEDEF_STRUCT(z_vtable_refs)
Z_TYPEDEF_STRUCT(z_bytes_ref)

struct z_vtable_refs {
  void (*inc_ref) (void *object);
  void (*dec_ref) (void *object);
};

struct z_bytes_ref {
  z_byte_slice_t slice;
  const z_vtable_refs_t *vtable;
  z_object_t *    object;
};

#define z_bytes_ref_slice(self)         (&((self)->slice))
#define z_bytes_ref_data(self)          ((self)->slice.data)
#define z_bytes_ref_size(self)          ((self)->slice.size)

#define z_bytes_ref_compare(a, b)                                       \
  z_byte_slice_compare(z_bytes_ref_slice(a), z_bytes_ref_slice(b))

void z_bytes_ref_reset   (z_bytes_ref_t *self);
void z_bytes_ref_set     (z_bytes_ref_t *self,
                          const z_byte_slice_t *slice,
                          const z_vtable_refs_t *vtable,
                          void *object);
void z_bytes_ref_acquire (z_bytes_ref_t *self,
                          const z_bytes_ref_t *other);
void z_bytes_ref_release (z_bytes_ref_t *self);


__Z_END_DECLS__

#endif /* !_Z_BYTES_REFS_H_ */
