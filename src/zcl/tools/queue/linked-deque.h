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

#ifndef _Z_LINKED_DEQUE_H_
#define _Z_LINKED_DEQUE_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/object.h>
#include <zcl/deque.h>

Z_TYPEDEF_STRUCT(z_linked_deque_node)
Z_TYPEDEF_STRUCT(z_linked_deque)

#define Z_CONST_LINKED_DEQUE(x)         ((const z_linked_deque_t *)(x))
#define Z_LINKED_DEQUE(x)               ((z_linked_deque_t *)(x))

struct z_linked_deque {
    __Z_OBJECT__(z_deque)

    z_linked_deque_node_t *head;
    z_linked_deque_node_t *tail;

    z_mem_free_t    data_free;
    void *          user_data;
};

z_linked_deque_t *z_linked_deque_alloc (z_linked_deque_t *self,
                                        z_memory_t *memory,
                                        z_mem_free_t data_free,
                                        void *user_data);
void              z_linked_deque_free  (z_linked_deque_t *self);

int z_linked_deque_is_empty (const z_linked_deque_t *self);

__Z_END_DECLS__

#endif /* !_Z_LINKED_DEQUE_H_ */
