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

#ifndef _Z_ATOMIC_H_
#define _Z_ATOMIC_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#if defined(Z_ATOMIC_GCC)
    #define Z_ATOMIC_SUPPORT
#endif

#include <zcl/macros.h>

#ifndef Z_ATOMIC_SUPPORT
    #include <zcl/thread.h>
#endif

Z_TYPEDEF_STRUCT(z_atomic_int)

struct z_atomic_int {
#ifndef Z_ATOMIC_SUPPORT
    z_spinlock_t lock;
#endif
    volatile int value;
};

#if defined(Z_ATOMIC_GCC)
    #define z_atomic_cas(p, e, n)                                           \
        __sync_bool_compare_and_swap(&((p)->value), (e), (n))

    #define z_atomic_add(p, v)      __sync_add_and_fetch(&((p)->value), (v))
    #define z_atomic_sub(p, v)      __sync_sub_and_fetch(&((p)->value), (v))
#else
    int z_atomic_cas (z_atomic_int_t *atom, int e, int n);
    int z_atomic_add (z_atomic_int_t *atom, int value);
    int z_atomic_sub (z_atomic_int_t *atom, int value);
#endif

#define z_atomic_incr(p)        z_atomic_add(p, 1)
#define z_atomic_decr(p)        z_atomic_sub(p, 1)

__Z_END_DECLS__

#endif /* !_Z_ATOMIC_H_ */

