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

#ifndef __DEQUE_PRIVATE_H__
#define __DEQUE_PRIVATE_H__

#include <raleighfs/errno.h>

#include <zcl/stream.h>
#include <zcl/deque.h>
#include <zcl/slice.h>

#define DEQUE_OBJECT(x)                     Z_CAST(deque_object_t, x)
#define DEQUE(x)                            Z_CAST(deque_t, x)

typedef struct deque_object {
    uint32_t length;
    uint8_t  data[1];
} deque_object_t;

typedef struct deque {
    z_deque_t deque;
    uint64_t  length;
    uint64_t  size;
} deque_t;

deque_t *           deque_alloc             (z_memory_t *memory);
void                deque_free              (z_memory_t *memory,
                                             deque_t *deque);

void                deque_clear             (deque_t *deque);

raleighfs_errno_t   deque_push_back         (deque_t *deque,
                                             const z_slice_t *value);
raleighfs_errno_t   deque_push_front        (deque_t *deque,
                                             const z_slice_t *value);

deque_object_t *    deque_pop_front         (deque_t *deque);
deque_object_t *    deque_pop_back          (deque_t *deque);

deque_object_t *    deque_get_front         (deque_t *deque);
deque_object_t *    deque_get_back          (deque_t *deque);

int                 deque_remove_front      (deque_t *deque);
int                 deque_remove_back       (deque_t *deque);

deque_object_t *    deque_object_alloc      (z_memory_t *memory,
                                             const z_slice_t *value);
void                deque_object_free       (z_memory_t *memory,
                                             deque_object_t *object);

#endif /* !__DEQUE_PRIVATE_H__ */

