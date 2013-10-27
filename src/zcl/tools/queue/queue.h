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

#ifndef _Z_QUEUE_H_
#define _Z_QUEUE_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/memory.h>
#include <zcl/object.h>

Z_TYPEDEF_STRUCT(z_vtable_queue)
Z_TYPEDEF_STRUCT(z_queue_interfaces)

#define Z_IMPLEMENT_QUEUE                 Z_IMPLEMENT(queue)

struct z_vtable_queue {
  int     (*push)   (void *self, void *data);
  void *  (*pop)    (void *self);
  void *  (*peek)   (void *self);
  size_t  (*size)   (void *self);
};

struct z_queue_interfaces {
  Z_IMPLEMENT_TYPE
  Z_IMPLEMENT_QUEUE
};

#define z_queue_push(self, data)      z_vtable_call(self, queue, push, data)
#define z_queue_pop(self)             z_vtable_call(self, queue, pop)
#define z_queue_peek(self)            z_vtable_call(self, queue, peek)
#define z_queue_size(self)            z_vtable_call(self, queue, size)

__Z_END_DECLS__

#endif /* _Z_QUEUE_H_ */
