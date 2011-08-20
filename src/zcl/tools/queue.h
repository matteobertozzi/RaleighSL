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

#ifndef _Z_QUEUE_H_
#define _Z_QUEUE_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/object.h>
#include <zcl/types.h>

Z_TYPEDEF_STRUCT(z_queue_node)
Z_TYPEDEF_STRUCT(z_queue)

#define Z_QUEUE(x)                                      Z_CAST(z_queue_t, x)

struct z_queue_node {
    z_queue_node_t *next;
    void *          data;
};

struct z_queue {
    Z_OBJECT_TYPE

    z_queue_node_t *head;
    z_queue_node_t *tail;

    z_mem_free_t    data_free;
    void *          user_data;
};

z_queue_t *     z_queue_alloc       (z_queue_t *queue,
                                     z_memory_t *memory,
                                     z_mem_free_t data_free,
                                     void *user_data);
void            z_queue_free        (z_queue_t *queue);

void            z_queue_clear       (z_queue_t *queue);

int             z_queue_push        (z_queue_t *queue,
                                     void *data);
void *          z_queue_pop         (z_queue_t *queue);
void *          z_queue_peek        (z_queue_t *queue);

int             z_queue_is_empty    (z_queue_t *queue);

__Z_END_DECLS__

#endif /* !_Z_QUEUE_H_ */

