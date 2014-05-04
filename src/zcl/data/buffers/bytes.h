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

#ifndef _Z_BYTES_H_
#define _Z_BYTES_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/memslice.h>
#include <zcl/memref.h>
#include <zcl/macros.h>
#include <zcl/string.h>

#define Z_BYTES(x)                  Z_CAST(z_bytes_t, x)

Z_TYPEDEF_STRUCT(z_bytes)

struct z_bytes {
  z_memslice_t slice;
  unsigned int refs;
};

#define z_bytes_slice(self)          (&((self)->slice))
#define z_bytes_size(self)           ((self)->slice.size)
#define z_bytes_data(self)           ((self)->slice.data)
#define z_bytes_equals(a, b)         (!z_bytes_compare(a, b))

#define z_bytes_from_slice(slice)                           \
  z_bytes_from_data((slice)->data, (slice)->size)

#define z_bytes_from_str(str)                                \
  z_bytes_from_data(str, z_strlen(str))

#define z_bytes_set(self, buf, size)                         \
  z_memcpy((self)->slice.data, buf, size)

#define z_bytes_compare(a, b)                                \
  z_byte_slice_compare(z_bytes_slice(a), z_bytes_slice(b))

extern const z_vtable_refs_t z_vtable_bytes_refs;

z_bytes_t *z_bytes_alloc   (size_t size);
z_bytes_t *z_bytes_acquire (z_bytes_t *self);
void       z_bytes_free    (z_bytes_t *self);

z_bytes_t *z_bytes_from_data      (const void *data, size_t size);

#endif /* !_Z_BYTES_H_ */
