/*
 *   Copyright 2011-2012 Matteo Bertozzi
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

#include <zcl/string.h>

int z_memsearch_u8 (const uint8_t *src,
                    size_t src_len,
                    const uint8_t *needle,
                    size_t needle_len,
                    z_extent_t *extent)
{
    size_t offset = 0;
    const uint8_t *p;
    size_t length;

    needle_len--;
    do {
        /* Try to find the first byte */
        if ((p = z_memchr(src, needle[0], src_len)) == NULL)
            return(1);

        offset += (p - src);
        src_len -= (p - src) + 1;
        src = (p + 1);

        /* try to match the shared part */
        length = z_min(src_len, needle_len);
        if (!z_memcmp(src, needle + 1, length)) {
            z_extent_set(extent, offset, length + 1);
            return(0);
        }
    } while (src_len > 0);

    return(1);
}

int z_memsearch_step_u8 (const uint8_t *src,
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