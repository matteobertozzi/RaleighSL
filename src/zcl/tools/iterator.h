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

#ifndef _Z_ITERATOR_H_
#define _Z_ITERATOR_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/comparer.h>
#include <zcl/dlink.h>
#include <zcl/type.h>

Z_TYPEDEF_STRUCT(z_iterator_interfaces)
Z_TYPEDEF_STRUCT(z_vtable_iterator)

#define __Z_ITERABLE__                      z_iterator_head_t __iter__;
#define Z_ITERATOR(x)                       Z_CAST(z_iterator_t, x)
#define Z_ITERATOR_HEAD(x)                  Z_ITERATOR(x)->head
#define Z_ITERATOR_VTABLE(x)                Z_ITERATOR_HEAD(x).vtable
#define Z_ITERATOR_ITERABLE(type, x)        Z_CONST_CAST(type, Z_ITERATOR_HEAD(x).iterable)

#define Z_IMPLEMENT_ITERATOR                Z_IMPLEMENT(iterator)

typedef struct z_iterator_head {
  const z_vtable_iterator_t *vtable;
  const void *iterable;
  z_dlink_node_t merge_list;
} z_iterator_head_t;

typedef struct z_iterator {
  z_iterator_head_t head;
  uint8_t data[512];
} z_iterator_t;

struct z_vtable_iterator {
  int     (*open)             (void *self, const void *object);
  void    (*close)            (void *self);
  void *  (*begin)            (void *self);
  void *  (*end)              (void *self);
  void *  (*seek)             (void *self, z_compare_t key_compare, const void *key);
  void *  (*skip)             (void *self, long n);
  void *  (*next)             (void *self);
  void *  (*prev)             (void *self);
  void *  (*current)          (void *self);
};

struct z_iterator_interfaces {
  Z_IMPLEMENT_TYPE
  Z_IMPLEMENT_ITERATOR
};

#define Z_ITERATOR_INIT(self, object)                                         \
  do {                                                                        \
    Z_ITERATOR_HEAD(self).vtable = z_vtable_interface(object, iterator);      \
    Z_ITERATOR_HEAD(self).iterable = object;                                  \
    z_dlink_init(&(Z_ITERATOR_HEAD(self).merge_list));                        \
  } while (0)

#define z_iterator_call(self, method, ...)                                    \
  Z_ITERATOR_VTABLE(self)->method(self, ##__VA_ARGS__)

#define z_iterator_object(self)             Z_ITERATOR_HEAD(self).iterable
#define z_iterator_set_object(self, obj)    z_iterator_object(self) = (obj)

#define z_iterator_open(self, object)                                         \
  z_vtable_interface(object, iterator)->open(self, object)

#define z_iterator_close(self)                                                \
  if (Z_ITERATOR_VTABLE(self)->close != NULL) z_iterator_call(self, close)

#define z_iterator_begin(self)              z_iterator_call(self, begin)
#define z_iterator_end(self)                z_iterator_call(self, end)
#define z_iterator_seek(self, cmpf, key)    z_iterator_call(self, seek, cmpf, key)
#define z_iterator_skip(self, n)            z_iterator_call(self, skip, n)
#define z_iterator_next(self)               z_iterator_call(self, next)
#define z_iterator_prev(self)               z_iterator_call(self, prev)
#define z_iterator_current(self)            z_iterator_call(self, current)

__Z_END_DECLS__

#endif /* _Z_ITERATOR_H_ */
