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

#ifndef _Z_READER_H_
#define _Z_READER_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/memref.h>
#include <zcl/slice.h>

#define __Z_READABLE__                z_reader_head_t __read__;
#define Z_READER(x)                   Z_CAST(z_reader_head_t, x)
#define Z_READER_VTABLE(x)            Z_READER(x)->vtable
#define Z_READER_READABLE(type, x)    Z_CONST_CAST(type, Z_READER(x)->readable)

#define Z_IMPLEMENT_READER            Z_IMPLEMENT(reader)

#define Z_VREADER_INIT(self, rvtable, object)                               \
  do {                                                                      \
    Z_READER(self)->vtable = rvtable;                                       \
    Z_READER(self)->readable = object;                                      \
  } while (0)

#define Z_READER_INIT(self, object)                                         \
  Z_VREADER_INIT(self, z_vtable_interface(object, reader), object)

Z_TYPEDEF_STRUCT(z_vtable_reader)
Z_TYPEDEF_STRUCT(z_reader_head)

struct z_reader_head {
  const z_vtable_reader_t *vtable;
  const void *readable;
};

struct z_vtable_reader {
  size_t          (*next)       (void *self, const uint8_t **data);
  void            (*backup)     (void *self, size_t count);
  size_t          (*available)  (void *self);
  const uint8_t * (*fetch)      (void *self, uint8_t *buffer, size_t length);

  int             (*open)       (void *self, const void *object);
  void            (*close)      (void *self);
};

#define z_reader_call(self, method, ...)                                  \
  Z_READER_VTABLE(self)->method(self, ##__VA_ARGS__)

#define z_reader_object(self)             Z_READER_HEAD(self).readable
#define z_reader_set_object(self, obj)    Z_READER_HEAD(self).readable = (obj)

#define z_reader_open(self, object)                                       \
  z_vtable_interface(object, reader)->open(self, object)

#define z_reader_close(self)              z_reader_call(self, close)
#define z_reader_next(self, data)         z_reader_call(self, next, data)
#define z_reader_backup(self, count)      z_reader_call(self, backup, count)
#define z_reader_available(self)          z_reader_call(self, available)

/* Reader util */
#define z_reader_skip(self, n)                                                 \
    z_v_reader_skip(Z_READER_VTABLE(self), self, n)

#define z_reader_fetch(self, buffer, length)                                   \
    z_v_reader_fetch(Z_READER_VTABLE(self), self, buffer, length)

/* Reader tokenize */
#define z_reader_search(self, needle, needle_len, slice)                       \
  z_v_reader_tokenize(Z_READER_VTABLE(self), self, needle, needle_len, extent)

#define z_reader_tokenize(self, needle, needle_len, slice)                     \
  z_v_reader_tokenize(Z_READER_VTABLE(self), self, needle, needle_len, slice)

/* Reader decoding */
#define z_reader_decode_field(self, field_id, length)                          \
  z_v_reader_decode_field(Z_READER_VTABLE(self), self, field_id, length)

#define z_reader_decode_int8(self, length, value)                              \
  z_v_reader_decode_int8(Z_READER_VTABLE(self), self, length, value)

#define z_reader_decode_uint8(self, length, value)                             \
  z_v_reader_decode_uint8(Z_READER_VTABLE(self), self, length, value)

#define z_reader_decode_int16(self, length, value)                             \
  z_v_reader_decode_int16(Z_READER_VTABLE(self), self, length, value)

#define z_reader_decode_uint16(self, length, value)                            \
  z_v_reader_decode_uint16(Z_READER_VTABLE(self), self, length, value)

#define z_reader_decode_int32(self, length, value)                             \
  z_v_reader_decode_int32(Z_READER_VTABLE(self), self, length, value)

#define z_reader_decode_uint32(self, length, value)                            \
  z_v_reader_decode_uint32(Z_READER_VTABLE(self), self, length, value)

#define z_reader_decode_int64(self, length, value)                             \
  z_v_reader_decode_int64(Z_READER_VTABLE(self), self, length, value)

#define z_reader_decode_uint64(self, length, value)                            \
  z_v_reader_decode_uint64(Z_READER_VTABLE(self), self, length, value)

#define z_reader_decode_bytes(self, length, value)                             \
  z_v_reader_decode_bytes(Z_READER_VTABLE(self), self, length, value)

#define z_reader_decode_string(self, length, value)                            \
  z_v_reader_decode_string(Z_READER_VTABLE(self), self, length, value)

#define z_v_reader_fetch(vtable, self, buffer, length)                         \
  ((Z_LIKELY((vtable)->fetch != NULL)) ?                                       \
    ((vtable)->fetch(self, buffer, length)) :                                  \
    z_v_reader_fetch_fallback(vtable, self, buffer, length))

/* Reader util */
int z_v_reader_skip  (const z_vtable_reader_t *vtable, void *self, size_t n);

const uint8_t *z_v_reader_fetch_fallback (const z_vtable_reader_t *vtable, void *self,
                                          uint8_t *buffer, size_t length);

/* Reader search/tokenize */
int z_v_reader_search (const z_vtable_reader_t *vtable, void *self,
                       const void *needle, size_t needle_len,
                       z_extent_t *extent);
size_t z_v_reader_tokenize (const z_vtable_reader_t *vtable, void *self,
                            const void *needle, size_t needle_len,
                            z_slice_t *slice);

/* Reader decoding */
int  z_v_reader_decode_field (const z_vtable_reader_t *vtable, void *self,
                              uint16_t *field_id, uint64_t *length);

int  z_v_reader_decode_int8      (const z_vtable_reader_t *vtable, void *self,
                                  size_t length, int8_t *value);
int  z_v_reader_decode_uint8     (const z_vtable_reader_t *vtable, void *self,
                                  size_t length, uint8_t *value);
int  z_v_reader_decode_int16     (const z_vtable_reader_t *vtable, void *self,
                                  size_t length, int16_t *value);
int  z_v_reader_decode_uint16    (const z_vtable_reader_t *vtable, void *self,
                                  size_t length, uint16_t *value);
int  z_v_reader_decode_int32     (const z_vtable_reader_t *vtable, void *self,
                                  size_t length, int32_t *value);
int  z_v_reader_decode_uint32    (const z_vtable_reader_t *vtable, void *self,
                                  size_t length, uint32_t *value);
int  z_v_reader_decode_int64     (const z_vtable_reader_t *vtable, void *self,
                                  size_t length, int64_t *value);
int  z_v_reader_decode_uint64    (const z_vtable_reader_t *vtable, void *self,
                                  size_t length, uint64_t *value);
int   z_v_reader_decode_bytes    (const z_vtable_reader_t *vtable, void *self,
                                  size_t length, z_memref_t *value);
int   z_v_reader_decode_string   (const z_vtable_reader_t *vtable, void *self,
                                  size_t length, z_memref_t *value);

__Z_END_DECLS__

#endif /* !_Z_READER_H_ */
