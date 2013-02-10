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

#ifndef _Z_MACROS_H_
#define _Z_MACROS_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>

#define Z_TYPEDEF_STRUCT(name)          typedef struct name name ## _t;
#define Z_TYPEDEF_UNION(name)           typedef union name name ## _t;
#define Z_TYPEDEF_ENUM(name)            typedef enum name name ## _t;

#define Z_PREFETCH(ptr)                 __builtin_prefetch((ptr))
#define Z_UNLIKELY(expr)                __builtin_expect(!!(expr), 0)
#define Z_LIKELY(expr)                  __builtin_expect(!!(expr), 1)

#define Z_CAST(type, x)                 ((type *)(x))
#define Z_CONST_CAST(type, x)           ((const type *)(x))

#define z_align_down(x, align)          ((x) & (-(align)))
#define z_align_up(x, align)            (((x) + ((align) - 1)) & (-(align)))

#define Z_ARGN_(_,_11,_10,_9,_8,_7,_6,_5,_4,_3,_2,_1,n,...)  n
#define Z_ARGN(...)             Z_ARGN_(0,__VA_ARGS__,11,10,9,8,7,6,5,4,3,2,1,0)

#define __ZFOLD_I(f, n)         f ## n
#define __ZFOLD_(f, x, n, ...)  __ZFOLD_I(f, n)(x, ##__VA_ARGS__)
#define Z_FOLD(f, x, ...)       __ZFOLD_(f, x, Z_ARGN(__VA_ARGS__), __VA_ARGS__)

#define __zmin_1(a, b)                  ((a) < (b) ? (a) : (b))
#define __zmin_2(a, b, c)               __zmin_1(__zmin_1(a, b), c)
#define __zmin_3(a, b, c, d)            __zmin_1(__zmin_2(a, b, c), d)
#define __zmin_4(a, b, c, d, e)         __zmin_1(__zmin_3(a, b, c, d), e)
#define __zmin_5(a, b, c, d, e, f)      __zmin_1(__zmin_4(a, b, c, d, e), f)
#define z_min(a, ...)                   (Z_FOLD(__zmin_, a, __VA_ARGS__))

#define __zmax_1(a, b)                  ((a) > (b) ? (a) : (b))
#define __zmax_2(a, b, c)               __zmax_1(__zmax_1(a, b), c)
#define __zmax_3(a, b, c, d)            __zmax_1(__zmax_2(a, b, c), d)
#define __zmax_4(a, b, c, d, e)         __zmax_1(__zmax_3(a, b, c, d), e)
#define __zmax_5(a, b, c, d, e, f)      __zmax_1(__zmax_4(a, b, c, d, e), f)
#define z_max(a, ...)                   (Z_FOLD(__zmax_, a, __VA_ARGS__))

#define __zsum_1(a, b)                  ((a) + (b))
#define __zsum_2(a, b, c)               __zsum_1(__zsum_1(a, b), c)
#define __zsum_3(a, b, c, d)            __zsum_1(__zsum_2(a, b, c), d)
#define __zsum_4(a, b, c, d, e)         __zsum_1(__zsum_3(a, b, c, d), e)
#define __zsum_5(a, b, c, d, e, f)      __zsum_1(__zsum_4(a, b, c, d, e), f)
#define z_sum(a, ...)                   (Z_FOLD(__zsum_, a, __VA_ARGS__))
#define z_avg(...)                      (z_sum(__VA_ARGS__)/Z_ARGN(__VA_ARGS__))

#define __zhas_0(x, a, p)               ((x) & (a) || (p))
#define __zhas_1(x, a)                  __zhas_0(x, a, 0)
#define __zhas_2(x, a, b)               __zhas_0(x, a, __zhas_1(x, b))
#define __zhas_3(x, a, b, c)            __zhas_0(x, a, __zhas_2(x, b, c))
#define __zhas_4(x, a, b, c, d)         __zhas_0(x, a, __zhas_3(x, b, c, d))
#define __zhas_5(x, a, b, c, d, e)      __zhas_0(x, a, __zhas_4(x, b, c, d, e))
#define z_has(x, ...)                   (Z_FOLD(__zhas_, x, __VA_ARGS__))

#define __zin_0(x, a, p)                ((x) == (a) || (p))
#define __zin_1(x, a)                   __zin_0(x, a, 0)
#define __zin_2(x, a, b)                __zin_0(x, a, __zin_1(x, b))
#define __zin_3(x, a, b, c)             __zin_0(x, a, __zin_2(x, b, c))
#define __zin_4(x, a, b, c, d)          __zin_0(x, a, __zin_3(x, b, c, d))
#define __zin_5(x, a, b, c, d, e)       __zin_0(x, a, __zin_4(x, b, c, d, e))
#define __zin_6(x, a, b, c, d, e, f)    __zin_0(x, a, __zin_5(x, b, c, d, e, f))
#define z_in(x, ...)                    (Z_FOLD(__zin_, x, __VA_ARGS__))

__Z_END_DECLS__

#endif /* !_Z_MACROS_H_ */
