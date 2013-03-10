/*
 *   Copyright 2011-2013 Matteo Bertozzi
 *
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

#include <zcl/coding.h>
#include <zcl/reader.h>
#include <zcl/extent.h>
#include <zcl/string.h>
#include <zcl/slice.h>

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
        if (!memcmp(src, needle, length)) {
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

static int __istok (uint8_t c, const uint8_t *tokens, unsigned int ntokens) {
    int r = 0;
    switch (ntokens) {
        case 4: r |= (c == tokens[3]);
        case 3: r |= (c == tokens[2]);
        case 2: r |= (c == tokens[1]);
        case 1: r |= (c == tokens[0]);
                break;
        default:
            while (ntokens--)
                r |= (c == *tokens++);
    }
    return(r);
}

static int __memtok_step (const uint8_t *src,
                          size_t src_len,
                          const uint8_t *tokens,
                          unsigned int ntokens,
                          z_extent_t *extent)
{
    const uint8_t *psrc = src;

    if (extent->length == 0) {
        /* Skip heading tokens */
        while (src_len > 0 && __istok(*psrc, tokens, ntokens)) {
            psrc++;
            src_len--;
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
        psrc++;
        src_len--;
    }

    extent->length += psrc - src;
    return(1);
}

int z_v_reader_search (const z_vtable_reader_t *vtable, void *self,
                       const void *needle, size_t needle_len,
                       z_extent_t *extent)
{
    const uint8_t *pneedle = (const uint8_t *)needle;
    uint8_t *buffer;
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

size_t z_v_reader_tokenize (const z_vtable_reader_t *vtable, void *self,
                            const void *needle, size_t needle_len,
                            z_slice_t *slice)
{
    const uint8_t *pneedle = (const uint8_t *)needle;
    z_extent_t extent;
    size_t nread = 0;
    uint8_t *buffer;
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
            z_slice_add(slice, buffer + offset, length);

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

int z_v_reader_skip (const z_vtable_reader_t *vtable, void *self, size_t length) {
    uint8_t *data;
    size_t n;
    while ((n = vtable->next(self, &data)) < length) {
        if (Z_UNLIKELY(!n)) return(-1);
        length -= n;
    }
    vtable->backup(self, n - length);
    return(0);
}

uint8_t *z_v_reader_fetch (const z_vtable_reader_t *vtable, void *self,
                           uint8_t *buffer, size_t length)
{
    uint8_t *pbuffer;
    uint8_t *data;
    size_t n;

    /* Data is required to be there */
    if (vtable->available == NULL || vtable->available(self) < length)
        return(NULL);

    if ((n = vtable->next(self, &data)) >= length) {
        vtable->backup(self, n - length);
        return(data);
    }

    z_memcpy(buffer, data, n);
    pbuffer = buffer + n;
    length -= n;
    while (length > 0) {
        if ((n = vtable->next(self, &data)) >= length) {
            vtable->backup(self, n - length);
            break;
        }

        z_memcpy(pbuffer, data, n);
        pbuffer += n;
        length -= n;
    }

    return(length > 0 ? NULL : buffer);
}

int z_v_reader_decode_field (const z_vtable_reader_t *vtable, void *self,
                             uint16_t *field_id, uint64_t *length)
{
    uint8_t *data;
    int elength;
    size_t n;

    if (!(n = vtable->next(self, &data)))
        return(-1);

    if ((elength = z_decode_field(data, n, field_id, length)) > 0) {
        vtable->backup(self, n - elength);
    } else {
        uint8_t buffer[16];
        /* Not in the first chunk */
        elength = -elength;
        vtable->backup(self, n);
        if (Z_UNLIKELY(!z_v_reader_fetch(vtable, self, buffer, elength)))
            return(-elength);
        elength = z_decode_field(buffer, elength, field_id, length);
    }

    return(elength);
}


int z_v_reader_decode_uint16 (const z_vtable_reader_t *vtable, void *self,
                              unsigned int length, uint16_t *value)
{
    uint8_t *data;
    size_t n;

    if ((n = vtable->next(self, &data)) >= length) {
        z_decode_uint16(data, length, value);
        vtable->backup(self, n - length);
    } else {
        uint8_t buffer[8];
        /* Not in the first chunk */
        vtable->backup(self, n);
        if (Z_UNLIKELY(!z_v_reader_fetch(vtable, self, buffer, length)))
            return(-length);
        z_decode_uint16(buffer, length, value);
    }

    return(0);
}


int z_v_reader_decode_uint32 (const z_vtable_reader_t *vtable, void *self,
                              unsigned int length, uint32_t *value)
{
    uint8_t *data;
    size_t n;

    if ((n = vtable->next(self, &data)) >= length) {
        z_decode_uint32(data, length, value);
        vtable->backup(self, n - length);
    } else {
        uint8_t buffer[8];
        /* Not in the first chunk */
        vtable->backup(self, n);
        if (Z_UNLIKELY(!z_v_reader_fetch(vtable, self, buffer, length)))
            return(-length);
        z_decode_uint32(buffer, length, value);
    }

    return(0);
}


int z_v_reader_decode_uint64 (const z_vtable_reader_t *vtable, void *self,
                              unsigned int length, uint64_t *value)
{
    uint8_t *data;
    size_t n;

    if ((n = vtable->next(self, &data)) >= length) {
        z_decode_uint64(data, length, value);
        vtable->backup(self, n - length);
    } else {
        uint8_t buffer[8];
        /* Not in the first chunk */
        vtable->backup(self, n);
        if (Z_UNLIKELY(!z_v_reader_fetch(vtable, self, buffer, length)))
            return(-length);
        z_decode_uint64(buffer, length, value);
    }

    return(0);
}
