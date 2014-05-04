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
 *  Reader search
 */
static int __memsearch_step (const uint8_t *src,
                             size_t src_len,
                             const uint8_t *needle,
                             size_t needle_len,
                             z_extent_t *extent)
{
  size_t length;

  /* Find the next chunk */
  if (extent->length > 0) {
    needle += extent->length;
    needle_len -= extent->length;

    length = z_min(src_len, needle_len);
    if (!z_memcmp(src, needle, length)) {
      extent->length += length;
      return(!!(needle_len - length));
    }

    extent->length = 0;
    needle -= extent->length;
    needle_len += extent->length;
  }

  /* Find the first chunk */
  length = extent->offset;
  if (z_memsearch_u8(src, src_len, needle, needle_len, extent)) {
    extent->offset = length + src_len;
    extent->length = 0;
  } else {
    extent->offset += length;
  }

  return(!!(extent->length - needle_len));
}

static int __istok (uint8_t c, const uint8_t *tokens, size_t ntokens) {
  int r = 0;
  switch (ntokens) {
    case 8: r  = (c == tokens[7]);
    case 7: r |= (c == tokens[6]);
    case 6: r |= (c == tokens[5]);
    case 5: r |= (c == tokens[4]);
    case 4: r |= (c == tokens[3]);
    case 3: r |= (c == tokens[2]);
    case 2: r |= (c == tokens[1]);
    case 1: r |= (c == tokens[0]);
            return(r);
  }

  while (ntokens--)
    r |= (c == *tokens++);
  return(r);
}

static int __memtok_step (const uint8_t *src,
                          size_t src_len,
                          const uint8_t *tokens,
                          size_t ntokens,
                          z_extent_t *extent)
{
  const uint8_t *psrc = src;

  if (extent->length == 0) {
    /* Skip heading tokens */
    while (src_len > 0 && __istok(*psrc, tokens, ntokens)) {
      ++psrc;
      --src_len;
    }

    extent->offset = psrc - src;
    src = psrc;
  }

  /* tok tok tok text tok */
  while (src_len > 0) {
    if (__istok(*psrc, tokens, ntokens)) {
      extent->length += psrc - src;
      return(0);
    }
    ++psrc;
    --src_len;
  }

  extent->length += psrc - src;
  return(1);
}

int z_v_reader_search (const z_vtable_reader_t *vtable, void *self,
                       const void *needle, size_t needle_len,
                       z_extent_t *extent)
{
  const uint8_t *pneedle = (const uint8_t *)needle;
  const uint8_t *buffer;
  size_t n;

  z_extent_reset(extent);
  while ((n = vtable->next(self, &buffer)) > 0) {
    size_t offset = extent->offset;
    if (!__memsearch_step(buffer, n, pneedle, needle_len, extent)) {
      n -= ((extent->offset - offset) + extent->length);
      vtable->backup(self, n);
      return(0);
    }
  }

  return(1);
}

/* ============================================================================
 *  Reader tokenize
 */
size_t z_v_reader_tokenize (const z_vtable_reader_t *vtable, void *self,
                            const void *needle, size_t needle_len,
                            z_slice_t *slice)
{
  const uint8_t *pneedle = (const uint8_t *)needle;
  const uint8_t *buffer;
  z_extent_t extent;
  size_t nread = 0;
  size_t n;

  z_slice_clear(slice); /* TODO: let decide to the user? */
  z_extent_reset(&extent);
  while ((n = vtable->next(self, &buffer)) > 0) {
    size_t offset = extent.offset;
    size_t length = extent.length;

    /* Find Token */
    int notok = __memtok_step(buffer, n, pneedle, needle_len, &extent);
    if (Z_LIKELY(extent.length > 0)) {
      /* Add to slice */
      offset = extent.offset - offset;
      length = extent.length - length;
      z_slice_append(slice, buffer + offset, length);

      if (!notok) {
        size_t last_read = (offset + length + 1);
        vtable->backup(self, n - last_read);
        nread += last_read;
        break;
      }
    } else {
      z_extent_reset(&extent);
    }

    nread += n;
  }

  return(nread);
}

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
