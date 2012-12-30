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

#ifndef _Z_DEQUE_H_
#define _Z_DEQUE_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/type.h>

Z_TYPEDEF_STRUCT(z_deque_interfaces)
Z_TYPEDEF_STRUCT(z_vtable_deque)

#define Z_IMPLEMENT_DEQUE             Z_IMPLEMENT(deque)

struct z_vtable_deque {
    int     (*push_back)        (void *self, void *element);
    int     (*push_front)       (void *self, void *element);
    void *  (*peek_back)        (void *self);
    void *  (*peek_front)       (void *self);
    void *  (*pop_back)         (void *self);
    void *  (*pop_front)        (void *self);
    void    (*clear)            (void *self);
    int     (*is_empty)         (const void *self);
};

struct z_deque_interfaces {
    Z_IMPLEMENT_TYPE
    Z_IMPLEMENT_DEQUE
};

#define z_deque_call(self, method, ...)                            \
    z_vtable_call(self, deque, method, ##__VA_ARGS__)

#define z_deque_push                        z_deque_push_back
#define z_deque_push_back(self, data)       z_deque_call(self, push_back, data)
#define z_deque_push_front(self, data)      z_deque_call(self, push_front, data)
#define z_deque_peek                        z_deque_peek_front
#define z_deque_peek_back(self)             z_deque_call(self, peek_back)
#define z_deque_peek_front(self)            z_deque_call(self, peek_front)
#define z_deque_pop                         z_deque_pop_front
#define z_deque_pop_back(self)              z_deque_call(self, pop_back)
#define z_deque_pop_front(self)             z_deque_call(self, pop_front)
#define z_deque_clear(self)                 z_deque_call(self, clear)
#define z_deque_is_empty(self)              z_deque_call(self, is_empty)

__Z_END_DECLS__

#endif /* !_Z_DEQUE_H_ */
