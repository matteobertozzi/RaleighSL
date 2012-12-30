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

#ifndef _Z_QUEUE_H_
#define _Z_QUEUE_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/type.h>

Z_TYPEDEF_STRUCT(z_queue_interfaces)
Z_TYPEDEF_STRUCT(z_vtable_queue)

#define Z_IMPLEMENT_QUEUE             Z_IMPLEMENT(queue)

struct z_vtable_queue {
    int     (*push)             (void *self, void *element);
    void *  (*peek)             (void *self);
    void *  (*pop)              (void *self);
    void    (*clear)            (void *self);
    int     (*is_empty)         (const void *self);
};

struct z_queue_interfaces {
    Z_IMPLEMENT_TYPE
    Z_IMPLEMENT_QUEUE
};

#define z_queue_call(self, method, ...)                            \
    z_vtable_call(self, queue, method, ##__VA_ARGS__)

#define z_queue_push(self, element)         z_queue_call(self, push, element)
#define z_queue_peek(self)                  z_queue_call(self, peek)
#define z_queue_pop(self)                   z_queue_call(self, pop)
#define z_queue_clear(self)                 z_queue_call(self, clear)
#define z_queue_is_empty(self)              z_queue_call(self, is_empty)

__Z_END_DECLS__

#endif /* !_Z_QUEUE_H_ */
