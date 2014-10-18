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

#include <zcl/field-coding.h>
#include <zcl/int-coding.h>
#include <zcl/memslice.h>
#include <zcl/reader.h>
#include <zcl/string.h>
#include <zcl/debug.h>
#include <zcl/bytes.h>
#include <zcl/bits.h>

/* ============================================================================
 *  Reader util
 */
int z_v_reader_skip (const z_vtable_reader_t *vtable, void *self, size_t length) {
  const uint8_t *data;
  size_t n;
  while ((n = vtable->next(self, &data)) < length) {
    if (Z_UNLIKELY(n == 0))
      return(-1);
    length -= n;
  }
  vtable->backup(self, n - length);
  return(0);
}

const uint8_t *z_v_reader_fetch_fallback (const z_vtable_reader_t *vtable, void *self,
                                          uint8_t *buffer, size_t length)
{
  const uint8_t *data;
  uint8_t *pbuffer;
  size_t n;

  if ((n = vtable->next(self, &data)) >= length) {
    vtable->backup(self, n - length);
    return(data);
  }

  length -= n;

  /* Data is required to be there */
  if (Z_UNLIKELY(vtable->available == NULL || vtable->available(self) < length)) {
    return(NULL);
  }

  z_memcpy(buffer, data, n);
  pbuffer = buffer + n;

  while (length > 0) {
    if ((n = vtable->next(self, &data)) >= length) {
      vtable->backup(self, n - length);
      z_memcpy(pbuffer, data, length);
      return(buffer);
    }

    z_memcpy(pbuffer, data, n);
    pbuffer += n;
    length -= n;
  }

  return(length > 0 ? NULL : buffer);
}

/* ============================================================================
 *  Reader decoding
 */
int z_v_reader_decode_field (const z_vtable_reader_t *vtable, void *self,
                             uint16_t *field_id, uint64_t *length)
{
  const uint8_t *data;
  int elength;
  size_t n;

  n = vtable->next(self, &data);
  if (Z_UNLIKELY(n == 0))
    return(-1);

  if ((elength = z_field_decode(data, n, field_id, length)) > 0) {
    vtable->backup(self, n - elength);
  } else {
    uint8_t buffer[16];

    /* Not in the first chunk */
    elength = -elength;
    vtable->backup(self, n);

    data = z_v_reader_fetch(vtable, self, buffer, elength);
    if (Z_UNLIKELY(data == NULL))
      return(-elength);

    elength = z_field_decode(buffer, elength, field_id, length);
  }

  return(elength);
}

int z_v_reader_decode_int8 (const z_vtable_reader_t *vtable, void *self,
                            size_t length, int8_t *value)
{
  uint8_t uvalue = 0;
  int r;
  r = z_v_reader_decode_uint8(vtable, self, length, &uvalue);
  *value = z_zigzag32_decode(uvalue);
  return r;
}

int  z_v_reader_decode_uint8  (const z_vtable_reader_t *vtable, void *self,
                               size_t length, uint8_t *value)
{
  const uint8_t *pbuf;

  pbuf = z_v_reader_fetch(vtable, self, value, length);
  if (Z_UNLIKELY(pbuf == NULL))
    return(1);

  *value = *pbuf;
  return(0);
}

int z_v_reader_decode_int16 (const z_vtable_reader_t *vtable, void *self,
                             size_t length, int16_t *value)
{
  uint16_t uvalue = 0;
  int r;
  r = z_v_reader_decode_uint16(vtable, self, length, &uvalue);
  *value = z_zigzag32_decode(uvalue);
  return r;
}

int z_v_reader_decode_uint16 (const z_vtable_reader_t *vtable, void *self,
                              size_t length, uint16_t *value)
{
  const uint8_t *pbuf;
  uint8_t buffer[2];

  pbuf = z_v_reader_fetch(vtable, self, buffer, length);
  if (Z_UNLIKELY(pbuf == NULL))
    return(1);

  z_uint16_decode(pbuf, length, value);
  return(0);
}

int z_v_reader_decode_int32 (const z_vtable_reader_t *vtable, void *self,
                             size_t length, int32_t *value)
{
  uint32_t uvalue = 0;
  int r;
  r = z_v_reader_decode_uint32(vtable, self, length, &uvalue);
  *value = z_zigzag32_decode(uvalue);
  return r;
}

int z_v_reader_decode_uint32 (const z_vtable_reader_t *vtable, void *self,
                              size_t length, uint32_t *value)
{
  const uint8_t *pbuf;
  uint8_t buffer[4];

  pbuf = z_v_reader_fetch(vtable, self, buffer, length);
  if (Z_UNLIKELY(pbuf == NULL))
    return(1);

  z_uint32_decode(pbuf, length, value);
  return(0);
}

int z_v_reader_decode_int64 (const z_vtable_reader_t *vtable, void *self,
                             size_t length, int64_t *value)
{
  uint64_t uvalue = 0;
  int r;
  r = z_v_reader_decode_uint64(vtable, self, length, &uvalue);
  *value = z_zigzag64_decode(uvalue);
  return r;
}

int z_v_reader_decode_uint64 (const z_vtable_reader_t *vtable, void *self,
                              size_t length, uint64_t *value)
{
  const uint8_t *pbuf;
  uint8_t buffer[8];

  pbuf = z_v_reader_fetch(vtable, self, buffer, length);
  if (Z_UNLIKELY(pbuf == NULL))
    return(1);

  z_uint64_decode(pbuf, length, value);
  return(0);
}

int z_v_reader_decode_bytes (const z_vtable_reader_t *vtable, void *self,
                             size_t length, z_memref_t *value)
{
  const uint8_t *pbuf;
  z_bytes_t *bytes;

  bytes = z_bytes_alloc(length);
  if (Z_UNLIKELY(bytes == NULL))
    return(2);

  pbuf = z_v_reader_fetch(vtable, self, (uint8_t *)bytes->slice.data, length);
  if (Z_UNLIKELY(pbuf == NULL)) {
    z_bytes_free(bytes);
    return(1);
  }

  if (pbuf != bytes->slice.data) {
    z_bytes_set(bytes, pbuf, length);
  }

  z_memref_set(value, &(bytes->slice), &z_vtable_bytes_refs, bytes);
  return(0);
}

int z_v_reader_decode_string (const z_vtable_reader_t *vtable, void *self,
                              size_t length, z_memref_t *value)
{
  return(z_v_reader_decode_bytes(vtable, self, length, value));
}
