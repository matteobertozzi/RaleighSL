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

#ifndef _Z_ATOMIC_H_
#define _Z_ATOMIC_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

//#if __STDC_NO_ATOMICS__ TODO

#if defined(Z_SYS_HAS_ATOMIC_GCC)
  #define z_atomic_set(p, v) ({                               \
    __asm__ volatile("" ::: "memory");                        \
    *(p) = (v);                                               \
    __asm__ volatile("lock addl $0x0,(%rsp)");                \
  })

  #define z_atomic_load(v) ({                                 \
    __asm__ __volatile__("" : : : "memory");                  \
    (*(volatile uint64_t *)(v));                              \
  })

  #define z_atomic_add_and_fetch(ptr, v) __sync_add_and_fetch(ptr, v)
  #define z_atomic_sub_and_fetch(ptr, v) __sync_sub_and_fetch(ptr, v)
  #define z_atomic_or_and_fetch(ptr, v)  __sync_or_and_fetch(ptr, v)
  #define z_atomic_and_and_fetch(ptr, v) __sync_and_and_fetch(ptr, v)
  #define z_atomic_xor_and_fetch(ptr, v) __sync_xor_and_fetch(ptr, v)
  #define z_atomic_fetch_and_add(ptr, v) __sync_fetch_and_add(ptr, v)
  #define z_atomic_fetch_and_sub(ptr, v) __sync_fetch_and_sub(ptr, v)

  #define z_atomic_cas(ptr, o, n)        __sync_bool_compare_and_swap(ptr, o, n)
  #define z_atomic_vcas(ptr, o, n)       __sync_val_compare_and_swap(ptr, o, n)
  #define z_atomic_synchronize()         __sync_synchronize()
  #define z_atomic_inc(ptr)              z_atomic_add_and_fetch(ptr, 1)
  #define z_atomic_dec(ptr)              z_atomic_sub_and_fetch(ptr, 1)
  #define z_atomic_add                   z_atomic_add_and_fetch
  #define z_atomic_sum                   z_atomic_sub_and_fetch

  #define z_busy_init(ptr)               *(ptr) = 0
  #define z_busy_try_lock(ptr)           !__sync_lock_test_and_set(ptr, 1)
  #define z_busy_lock(ptr)               while (!z_busy_try_lock(ptr));
  #define z_busy_unlock(ptr)             __sync_lock_release(ptr)
#else
  #error "No atomic support"
#endif

#define z_atomic_cas_loop(ptrval, curval, expval, newval, __code__)           \
  do {                                                                        \
    (curval) = z_atomic_load(ptrval);                                         \
    do {                                                                      \
      do __code__ while (0);                                                  \
    } while (((curval) = z_atomic_vcas(ptrval, expval, newval)) != (expval)); \
  } while (0)

#define z_atomic_cas_retry_loop(ptrval, expval, newval)                       \
  while (z_atomic_vcas(ptrval, expval, newval) != (expval))

__Z_END_DECLS__

#endif /* _Z_ATOMIC_H_ */