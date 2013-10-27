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
#include <zcl/memory.h>

Z_TYPEDEF_STRUCT(z_atomic_stack)
Z_TYPEDEF_STRUCT(z_atomic_queue)

#if defined(Z_SYS_HAS_ATOMIC_GCC)
  #define z_atomic_set(ptr, v)           (*(ptr)) = (v)
  #define z_atomic_load(ptr)             __sync_fetch_and_add(ptr, 0)
  #define z_atomic_add_and_fetch(ptr, v) __sync_add_and_fetch(ptr, v)
  #define z_atomic_sub_and_fetch(ptr, v) __sync_sub_and_fetch(ptr, v)
  #define z_atomic_fetch_and_add(ptr, v) __sync_fetch_and_add(ptr, v)
  #define z_atomic_fetch_and_sub(ptr, v) __sync_fetch_and_sub(ptr, v)
  #define z_atomic_cas(ptr, o, n)        __sync_bool_compare_and_swap(ptr, o, n)
  #define z_atomic_vcas(ptr, o, n)       __sync_val_compare_and_swap(ptr, o, n)
  #define z_atomic_inc(ptr)              z_atomic_add_and_fetch(ptr, 1)
  #define z_atomic_dec(ptr)              z_atomic_sub_and_fetch(ptr, 1)
#else
  #error "No atomic support"
#endif

#define z_atomic_cas_loop(ptrval, curval, expval, newval, __code__)           \
  do {                                                                        \
    (curval) = z_atomic_load(ptrval);                                         \
    while (1) {                                                               \
      do __code__ while (0);                                                  \
      if (((curval) = z_atomic_vcas(ptrval, expval, newval)) == (expval))     \
        break;                                                                \
    }                                                                         \
  } while (0)


struct z_atomic_stack {
  uint8_t data[64];
};

struct z_atomic_queue {
  uint8_t data[128];
};

int   z_atomic_stack_alloc  (z_atomic_stack_t *stack);
int   z_atomic_stack_free   (z_atomic_stack_t *stack);
int   z_atomic_stack_push   (z_atomic_stack_t *stack, void *data);
void *z_atomic_stack_pop    (z_atomic_stack_t *stack);


int   z_atomic_queue_alloc  (z_atomic_queue_t *queue);
int   z_atomic_queue_free   (z_atomic_queue_t *queue);
int   z_atomic_queue_push   (z_atomic_queue_t *queue, void *data);
void *z_atomic_queue_pop    (z_atomic_queue_t *queue);


__Z_END_DECLS__

#endif /* _Z_ATOMIC_H_ */
