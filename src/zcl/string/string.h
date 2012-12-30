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

#ifndef _Z_STRING_H_
#define _Z_STRING_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/extent.h>

#include <string.h>

#define z_memchr            memchr
#define z_memmem            memmem
#define z_memmove           memmove
#define z_memcpy            memcpy
#define z_memset            memset
#define z_memcmp            memcmp

#define z_memzero(dst, n)   z_memset(dst, 0, n)

#define z_memsearch(src, src_len, needle, needle_len, extent)               \
    z_memsearch_u8(Z_CONST_UINT8(src), src_len,                             \
                   Z_CONST_UINT8(needle), needle_len, extent)

int z_memsearch_u8 (const uint8_t *src,
                    size_t src_len,
                    const uint8_t *needle,
                    size_t needle_len,
                    z_extent_t *extent);

#define z_strcmp          strcmp
#define z_strlen          strlen

size_t  z_strlcpy         (char *dst, const char *src, size_t size);

__Z_END_DECLS__

#endif /* !_Z_STRING_H_ */

