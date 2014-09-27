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

#ifndef _Z_DATA_MEM_SLICE_H_
#define _Z_DATA_MEM_SLICE_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <string.h>

#include <zcl/macros.h>
#include <zcl/memutil.h>

Z_TYPEDEF_STRUCT(z_memslice)

#define Z_CONST_MEMSLICE(x)                Z_CONST_CAST(z_memslice_t, x)
#define Z_MEMSLICE(x)                      Z_CAST(z_memslice_t, x)

struct z_memslice {
  uint32_t size;
  uint32_t uflags32;
  uint8_t *data;
};

#define z_memslice_set(slice, data_, size_)                                  \
  do {                                                                       \
    (slice)->data = ((uint8_t *)(data_));                                    \
    (slice)->size = size_;                                                   \
  } while (0)

#define z_memslice_copy(self, slice)                                         \
  z_memslice_set(self, (slice)->data, (slice)->size)

#define z_memslice_clear(slice)                                              \
  z_memslice_set(slice, NULL, 0)

#define z_memslice_is_empty(slice)                                           \
  (Z_CONST_MEMSLICE(slice)->size == 0)

#define z_memslice_is_not_empty(slice)                                       \
  (Z_CONST_MEMSLICE(slice)->size > 0)

#define z_memslice_starts_with(slice, blob, n)                               \
  ((Z_CONST_MEMSLICE(slice)->size >= (n)) &&                                 \
   !z_memcmp(Z_CONST_MEMSLICE(slice)->data, blob, n))

#define z_memcpy_slice(dst, slice)                                           \
  z_memcpy(dst, (slice)->data, (slice)->size)

#define z_memslice_compare(self, other)                                      \
  z_mem_compare((self)->data, (self)->size, (other)->data, (other)->size)

__Z_END_DECLS__

#endif /* _Z_DATA_MEM_SLICE_H_ */
