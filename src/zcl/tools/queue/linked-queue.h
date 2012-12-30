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

#ifndef _Z_LINKED_QUEUE_H_
#define _Z_LINKED_QUEUE_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/object.h>
#include <zcl/queue.h>

Z_TYPEDEF_STRUCT(z_linked_queue_node)
Z_TYPEDEF_STRUCT(z_linked_queue)

#define Z_CONST_LINKED_QUEUE(x)         ((const z_linked_queue_t *)(x))
#define Z_LINKED_QUEUE(x)               ((z_linked_queue_t *)(x))

struct z_linked_queue {
    __Z_OBJECT__(z_queue)

    z_linked_queue_node_t *head;
    z_linked_queue_node_t *tail;

    z_mem_free_t    data_free;
    void *          user_data;
};

__Z_END_DECLS__

#endif /* !_Z_LINKED_QUEUE_H_ */
