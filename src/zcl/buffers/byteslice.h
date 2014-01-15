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

#ifndef _Z_BYTE_SLICE_H_
#define _Z_BYTE_SLICE_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

Z_TYPEDEF_STRUCT(z_byte_slice)

#define Z_CONST_BYTE_SLICE(x)                Z_CONST_CAST(z_byte_slice_t, x)
#define Z_BYTE_SLICE(x)                      Z_CAST(z_byte_slice_t, x)

struct z_byte_slice {
  uint32_t size;
  uint8_t *data;
};

#define z_byte_slice_set(slice, data_, size_)                                 \
  do {                                                                        \
    (slice)->data = ((uint8_t *)data_);                                       \
    (slice)->size = size_;                                                    \
  } while (0)

#define z_byte_slice_copy(self, slice)                                        \
  z_byte_slice_set(self, (slice)->data, (slice)->size)

#define z_byte_slice_clear(slice)                                             \
  z_byte_slice_set(slice, NULL, 0)

#define z_byte_slice_is_empty(slice)                                          \
  (Z_CONST_BYTE_SLICE(slice)->size == 0)

#define z_byte_slice_is_not_empty(slice)                                      \
  (Z_CONST_BYTE_SLICE(slice)->size > 0)

#define z_byte_slice_starts_with(slice, blob, n)                              \
  ((Z_CONST_BYTE_SLICE(slice)->size >= (n)) &&                                \
   !z_memcmp(Z_CONST_BYTE_SLICE(slice)->data, blob, n))

#define z_byte_slice_memcpy(dst, slice)                                       \
  z_memcpy(dst, (slice)->data, (slice)->size)

int z_byte_slice_strcmp  (const z_byte_slice_t *self, const char *str);
int z_byte_slice_compare (const z_byte_slice_t *self, const z_byte_slice_t *other);

__Z_END_DECLS__

#endif /* _Z_BYTE_SLICE_H_ */
