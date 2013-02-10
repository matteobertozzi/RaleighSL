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

#ifndef _Z_ATOMIC_H_
#define _Z_ATOMIC_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/memory.h>

/*
#if defined(Z_CPU_HAS_DWCAS)
  #define Z_CPU_HAS_LOCK_FREE_SUPPORT
#endif
*/

Z_TYPEDEF_STRUCT(z_atomic_stack)
Z_TYPEDEF_STRUCT(z_atomic_queue)

#define z_atomic_get(ptr)         __sync_fetch_and_add(ptr, 0)
#define z_atomic_add(ptr, v)      __sync_add_and_fetch(ptr, v)
#define z_atomic_sub(ptr, v)      __sync_sub_and_fetch(ptr, v)

#define z_atomic_inc(ptr)         z_atomic_add(ptr, 1)
#define z_atomic_dec(ptr)         z_atomic_sub(ptr, 1)

z_atomic_stack_t *  z_atomic_stack_alloc  (z_memory_t *memory);
int                 z_atomic_stack_free   (z_atomic_stack_t *stack);
int                 z_atomic_stack_push   (z_atomic_stack_t *stack,
                                           void *data);
void *              z_atomic_stack_pop    (z_atomic_stack_t *stack);


z_atomic_queue_t *  z_atomic_queue_alloc  (z_memory_t *memory);
int                 z_atomic_queue_free   (z_atomic_queue_t *queue);
int                 z_atomic_queue_push   (z_atomic_queue_t *queue,
                                           void *data);
void *              z_atomic_queue_pop    (z_atomic_queue_t *queue);

__Z_END_DECLS__

#endif /* !_Z_ATOMIC_H_ */
