/*
 *   Copyright 2011 Matteo Bertozzi
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

#ifndef _Z_MACROS_H_
#define _Z_MACROS_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <stdlib.h>
#include <stdint.h>

#define Z_TYPEDEF_CONST_STRUCT(name)    typedef const struct name name ## _t;
#define Z_TYPEDEF_STRUCT(name)          typedef struct name name ## _t;
#define Z_TYPEDEF_UNION(name)           typedef union name name ## _t;

#define Z_PREFETCH(ptr)                 __builtin_prefetch((ptr))
#define Z_UNLIKELY(expr)                __builtin_expect(!!(expr), 0)
#define Z_LIKELY(expr)                  __builtin_expect(!!(expr), 1)

#define Z_CAST(type, x)                 ((type *)(x))
#define Z_CONST_CAST(type, x)           ((const type *)(x))

#define Z_INT_PTR(x)                    Z_CAST(int, x)
#define Z_INT8_PTR(x)                   Z_CAST(int8_t, x)
#define Z_INT16_PTR(x)                  Z_CAST(int16_t, x)
#define Z_INT32_PTR(x)                  Z_CAST(int32_t, x)
#define Z_INT64_PTR(x)                  Z_CAST(int64_t, x)

#define Z_UINT_PTR(x)                   Z_CAST(unsigned int, x)
#define Z_UINT8_PTR(x)                  Z_CAST(uint8_t, x)
#define Z_UINT16_PTR(x)                 Z_CAST(uint16_t, x)
#define Z_UINT32_PTR(x)                 Z_CAST(uint32_t, x)
#define Z_UINT64_PTR(x)                 Z_CAST(uint64_t, x)

#define Z_INT_PTR_VALUE(x)              (*Z_INT_PTR(x))
#define Z_INT8_PTR_VALUE(x)             (*Z_INT8_PTR(x))
#define Z_INT16_PTR_VALUE(x)            (*Z_INT16_PTR(x))
#define Z_INT32_PTR_VALUE(x)            (*Z_INT32_PTR(x))
#define Z_INT64_PTR_VALUE(x)            (*Z_INT64_PTR(x))

#define Z_UINT_PTR_VALUE(x)             (*Z_UINT_PTR(x))
#define Z_UINT8_PTR_VALUE(x)            (*Z_UINT8_PTR(x))
#define Z_UINT16_PTR_VALUE(x)           (*Z_UINT16_PTR(x))
#define Z_UINT32_PTR_VALUE(x)           (*Z_UINT32_PTR(x))
#define Z_UINT64_PTR_VALUE(x)           (*Z_UINT64_PTR(x))

#define Z_CONST_INT_PTR(x)              Z_CONST_CAST(int, x)
#define Z_CONST_INT8_PTR(x)             Z_CONST_CAST(int8_t, x)
#define Z_CONST_INT16_PTR(x)            Z_CONST_CAST(int16_t, x)
#define Z_CONST_INT32_PTR(x)            Z_CONST_CAST(int32_t, x)
#define Z_CONST_INT64_PTR(x)            Z_CONST_CAST(int64_t, x)

#define Z_CONST_UINT_PTR(x)             Z_CONST_CAST(unsigned int, x)
#define Z_CONST_UINT8_PTR(x)            Z_CONST_CAST(uint8_t, x)
#define Z_CONST_UINT16_PTR(x)           Z_CONST_CAST(uint16_t, x)
#define Z_CONST_UINT32_PTR(x)           Z_CONST_CAST(uint32_t, x)
#define Z_CONST_UINT64_PTR(x)           Z_CONST_CAST(uint64_t, x)

#define Z_CONST_INT_PTR_VALUE(x)        (*Z_CONST_INT_PTR(x))
#define Z_CONST_INT8_PTR_VALUE(x)       (*Z_CONST_INT8_PTR(x))
#define Z_CONST_INT16_PTR_VALUE(x)      (*Z_CONST_INT16_PTR(x))
#define Z_CONST_INT32_PTR_VALUE(x)      (*Z_CONST_INT32_PTR(x))
#define Z_CONST_INT64_PTR_VALUE(x)      (*Z_CONST_INT64_PTR(x))

#define Z_CONST_UINT_PTR_VALUE(x)       (*Z_CONST_UINT_PTR(x))
#define Z_CONST_UINT8_PTR_VALUE(x)      (*Z_CONST_UINT8_PTR(x))
#define Z_CONST_UINT16_PTR_VALUE(x)     (*Z_CONST_UINT16_PTR(x))
#define Z_CONST_UINT32_PTR_VALUE(x)     (*Z_CONST_UINT32_PTR(x))
#define Z_CONST_UINT64_PTR_VALUE(x)     (*Z_CONST_UINT64_PTR(x))

#define z_align_down(x, align)          ((x) & (-(align)))
#define z_align_up(x, align)            ((((x) + ((align) - 1)) & (-(align))))

#define z_rotl32(x, r)                  (((x) << (r)) | ((x) >> (32 - (r))))
#define z_rotl64(x, r)                  (((x) << (r)) | ((x) >> (64 - (r))))
#define z_rotr32(x, r)                  (((x) >> (r)) | ((x) << (32 - (r))))
#define z_rotr64(x, r)                  (((x) >> (r)) | ((x) << (64 - (r))))

#define z_min(a, b)                     ((a) < (b) ? (a) : (b))
#define z_max(a, b)                     ((a) > (b) ? (a) : (b))

#define z_bit_test(x, n)                (!!((x) & (1 << (n))))

#define z_bit_set(x, n)                                                     \
    do { (x) |= 1 << (n); } while (0)

#define z_bit_clear(x, n)                                                   \
    do { (x) &= ~(1 << (n)); } while (0)

#define z_bit_toggle(x, n)                                                  \
    do { (x) ^= 1 << (n); } while (0)

#define z_bit_change(x, n, v)                                               \
    do { (x) = (((x) & ~(1 << (n))) | ((!!v) << (n))); } while (0)

__Z_END_DECLS__

#endif /* !_Z_MACROS_H_ */

