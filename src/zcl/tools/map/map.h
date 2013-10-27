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


#ifndef _Z_MAP_H_
#define _Z_MAP_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/byteslice.h>
#include <zcl/comparer.h>
#include <zcl/iterator.h>
#include <zcl/dlink.h>
#include <zcl/type.h>

Z_TYPEDEF_STRUCT(z_sorted_map_interfaces)
Z_TYPEDEF_STRUCT(z_map_interfaces)

Z_TYPEDEF_STRUCT(z_vtable_map_iterator)
Z_TYPEDEF_STRUCT(z_vtable_sorted_map)
Z_TYPEDEF_STRUCT(z_vtable_map)

Z_TYPEDEF_STRUCT(z_map_iterator_head)
Z_TYPEDEF_STRUCT(z_map_iterator)
Z_TYPEDEF_STRUCT(z_map_merger)
Z_TYPEDEF_STRUCT(z_map_entry)

#define Z_IMPLEMENT_MAP_ITERATOR           Z_IMPLEMENT(map_iterator)
#define Z_IMPLEMENT_SORTED_MAP             Z_IMPLEMENT(sorted_map)
#define Z_IMPLEMENT_MAP                    Z_IMPLEMENT(map)

#define __Z_MAP_ITERABLE__                 z_map_iterator_head_t __iter__;
#define Z_MAP_ITERATOR(x)                  Z_CAST(z_map_iterator_t, x)
#define Z_MAP_ITERATOR_HEAD(x)             Z_MAP_ITERATOR(x)->head
#define Z_MAP_ITERATOR_VTABLE(x)           Z_MAP_ITERATOR_HEAD(x).vtable
#define Z_MAP_ITERATOR_ITERABLE(type, x)   Z_CONST_CAST(type, Z_MAP_ITERATOR_HEAD(x).iterable)

struct z_vtable_map {
  void * (*get)            (void *self, const void *key);
  int    (*put)            (void *self, void *key_value);
  void * (*pop)            (void *self, const void *key);
  int    (*remove)         (void *self, const void *key);
  void   (*clear)          (void *self);
  int    (*size)           (const void *self);

  void * (*get_custom)     (void *self, z_compare_t key_cmp, const void *key);
  void * (*pop_custom)     (void *self, z_compare_t key_cmp, const void *key);
  int    (*remove_custom)  (void *self, z_compare_t key_cmp, const void *key);
};

struct z_vtable_sorted_map {
  void *  (*min)              (const void *self);
  void *  (*max)              (const void *self);
  void *  (*ceil)             (const void *self, const void *key);
  void *  (*floor)            (const void *self, const void *key);
};

struct z_vtable_map_iterator {
  int   (*begin)    (void *self);
  int   (*next)     (void *self);
  int   (*seek)     (void *self, const z_byte_slice_t *key);

  const z_map_entry_t *  (*current)  (void *self);
};

struct z_map_interfaces {
  Z_IMPLEMENT_TYPE
  Z_IMPLEMENT_ITERATOR
  Z_IMPLEMENT_MAP
};

struct z_sorted_map_interfaces {
  Z_IMPLEMENT_TYPE
  Z_IMPLEMENT_ITERATOR
  Z_IMPLEMENT_MAP
  Z_IMPLEMENT_SORTED_MAP
};

struct z_map_entry {
  z_byte_slice_t key;
  z_byte_slice_t value;
};

struct z_map_merger {
  z_dlink_node_t merge_list;
  z_map_iterator_t *smallest_iter;
};

struct z_map_iterator_head {
  const z_vtable_map_iterator_t *vtable;
  const void *iterable;
  z_dlink_node_t merge_list;
};

struct z_map_iterator {
  z_map_iterator_head_t head;
  uint8_t data[512];
};

#define z_map_call(self, method, ...)                            \
    z_vtable_call(self, map, method, ##__VA_ARGS__)

#define z_sorted_map_call(self, method, ...)                     \
    z_vtable_call(self, sorted_map, method, ##__VA_ARGS__)

#define z_map_get(self, key)               z_map_call(self, get, key)
#define z_map_put(self, key_value)         z_map_call(self, put, key_value)
#define z_map_pop(self, key)               z_map_call(self, pop, key)
#define z_map_remove(self, key)            z_map_call(self, remove, key)
#define z_map_clear(self)                  z_map_call(self, clear)

#define z_map_size(self)                   z_map_call(self, size)
#define z_map_contains(self, key)          (z_map_get(self, key) != NULL)

#define z_sorted_map_min(self)             z_sorted_map_call(self, min)
#define z_sorted_map_max(self)             z_sorted_map_call(self, max)
#define z_sorted_map_ceil(self, key)       z_sorted_map_call(self, ceil, key)
#define z_sorted_map_floor(self, key)      z_sorted_map_call(self, floor, key)

#define Z_MAP_ITERATOR_INIT(self, vtable_)                                    \
  do {                                                                        \
    Z_MAP_ITERATOR_HEAD(self).vtable = (vtable_);                             \
    z_dlink_init(&(Z_MAP_ITERATOR_HEAD(self).merge_list));                    \
  } while (0)

#define z_map_iterator_call(self, method, ...)                                \
  Z_MAP_ITERATOR_VTABLE(self)->method(self, ##__VA_ARGS__)

#define z_map_iterator_begin(self)          z_map_iterator_call(self, begin)
#define z_map_iterator_next(self)           z_map_iterator_call(self, next)
#define z_map_iterator_current(self)        z_map_iterator_call(self, current)
#define z_map_iterator_seek(self, key)      z_map_iterator_call(self, seek)

void z_map_merger_open (z_map_merger_t *self);
int  z_map_merger_add  (z_map_merger_t *self, z_map_iterator_t *iter);
const z_map_entry_t *z_map_merger_next (z_map_merger_t *self);

__Z_END_DECLS__

#endif /* _Z_MAP_H_ */
