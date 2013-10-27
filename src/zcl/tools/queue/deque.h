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

#ifndef _Z_DEQUE_H_
#define _Z_DEQUE_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/memory.h>
#include <zcl/object.h>

Z_TYPEDEF_STRUCT(z_vtable_deque)
Z_TYPEDEF_STRUCT(z_deque_interfaces)

#define Z_IMPLEMENT_DEQUE                 Z_IMPLEMENT(deque)

struct z_vtable_deque {
  int     (*push_front)  (void *self, void *data);
  int     (*push_back)   (void *self, void *data);
  void *  (*pop_front)   (void *self);
  void *  (*pop_back)    (void *self);
  void *  (*peek_front)  (void *self);
  void *  (*peek_back)   (void *self);
  size_t  (*size)        (void *self);
};

struct z_deque_interfaces {
  Z_IMPLEMENT_TYPE
  Z_IMPLEMENT_DEQUE
};

#define z_deque_push_front(self, data)                                      \
  z_vtable_call(self, deque, push_front, data)

#define z_deque_push_back(self, data)                                       \
  z_vtable_call(self, deque, push_front, data)

#define z_deque_pop_front(self, data)                                       \
  z_vtable_call(self, deque, pop_front, data)

#define z_deque_pop_back(self, data)                                        \
  z_vtable_call(self, deque, pop_back, data)

#define z_deque_peek_front(self, data)                                      \
  z_vtable_call(self, deque, peek_front, data)

#define z_deque_peek_back(self, data)                                       \
  z_vtable_call(self, deque, peek_back, data)

#define z_deque_size(self)                                                  \
  z_vtable_call(self, deque, size)

__Z_END_DECLS__

#endif /* _Z_DEQUE_H_ */
