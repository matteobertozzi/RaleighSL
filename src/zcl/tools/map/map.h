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

#ifndef _Z_MAP_H_
#define _Z_MAP_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/comparer.h>
#include <zcl/type.h>

Z_TYPEDEF_STRUCT(z_sorted_map_interfaces)
Z_TYPEDEF_STRUCT(z_map_interfaces)

Z_TYPEDEF_STRUCT(z_vtable_sorted_map)
Z_TYPEDEF_STRUCT(z_vtable_map)

#define Z_IMPLEMENT_SORTED_MAP      Z_IMPLEMENT(sorted_map)
#define Z_IMPLEMENT_MAP             Z_IMPLEMENT(map)

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

__Z_END_DECLS__

#endif /* !_Z_MAP_H_ */
