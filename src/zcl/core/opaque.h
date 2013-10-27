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

#ifndef _Z_OPAQUE_H_
#define _Z_OPAQUE_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

/* Generic Data Type */
#define Z_OPAQUE(x)                   Z_CAST(z_opaque_t, x)
#define Z_OPAQUE_FD(x)                (Z_OPAQUE(x)->fd)
#define Z_OPAQUE_U32(x)               (Z_OPAQUE(x)->u32)
#define Z_OPAQUE_U64(x)               (Z_OPAQUE(x)->u64)
#define Z_OPAQUE_PTR(x, type)         Z_CAST(type, (Z_OPAQUE(x)->ptr))

#define Z_OPAQUE_SET_FD(x, value)     (Z_OPAQUE(x)->fd = (value))
#define Z_OPAQUE_SET_U32(x, value)    (Z_OPAQUE(x)->u32 = (value))
#define Z_OPAQUE_SET_U64(x, value)    (Z_OPAQUE(x)->u64 = (value))
#define Z_OPAQUE_SET_PTR(x, value)    (Z_OPAQUE(x)->ptr = (value))

typedef union z_opaque {
    void *   ptr;
    int      fd;
    uint32_t u32;
    uint64_t u64;
} z_opaque_t;

__Z_END_DECLS__

#endif /* _Z_OPAQUE_H_ */
