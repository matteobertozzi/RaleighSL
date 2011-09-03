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

#ifndef _Z_DEQUE_H_
#define _Z_DEQUE_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/object.h>
#include <zcl/types.h>

Z_TYPEDEF_STRUCT(z_deque_node)
Z_TYPEDEF_STRUCT(z_deque)

#define Z_DEQUE(x)                      Z_CAST(z_deque_t, x)

struct z_deque_node {
    z_deque_node_t *next;
    z_deque_node_t *prev;
    void *          data;
};

struct z_deque {
    Z_OBJECT_TYPE

    z_deque_node_t *head;
    z_deque_node_t *tail;

    z_mem_free_t    data_free;
    void *          user_data;
};

z_deque_t *     z_deque_alloc       (z_deque_t *deque,
                                     z_memory_t *memory,
                                     z_mem_free_t data_free,
                                     void *user_data);
void            z_deque_free        (z_deque_t *deque);

void            z_deque_clear       (z_deque_t *deque);

#define         z_deque_push        z_deque_push_back
int             z_deque_push_front  (z_deque_t *deque,
                                     void *data);
int             z_deque_push_back   (z_deque_t *deque,
                                     void *data);

#define         z_deque_pop         z_deque_pop_front
void *          z_deque_pop_front   (z_deque_t *deque);
void *          z_deque_pop_back    (z_deque_t *deque);

#define         z_deque_peek        z_deque_peek_front
void *          z_deque_peek_front  (z_deque_t *deque);
void *          z_deque_peek_back   (z_deque_t *deque);

int             z_deque_is_empty    (z_deque_t *deque);

__Z_END_DECLS__

#endif /* !_Z_deque_H_ */

