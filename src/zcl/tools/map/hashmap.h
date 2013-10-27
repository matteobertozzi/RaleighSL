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

#ifndef _Z_HASH_MAP_H_
#define _Z_HASH_MAP_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/iterator.h>
#include <zcl/object.h>
#include <zcl/hash.h>
#include <zcl/map.h>

#define Z_CONST_HASH_MAP_PLUG(x)     Z_CONST_CAST(z_hash_map_plug_t, x)
#define Z_HASH_MAP_ITERATOR(x)       Z_CAST(z_hash_map_iterator_t, x)
#define Z_CONST_HASH_MAP(x)          Z_CONST_CAST(z_hash_map_t, x)
#define Z_HASH_MAP(x)                Z_CAST(z_hash_map_t, x)

Z_TYPEDEF_STRUCT(z_hash_map_iterator)
Z_TYPEDEF_STRUCT(z_hash_map_plug)
Z_TYPEDEF_STRUCT(z_hash_map)

struct z_hash_map_iterator {
  __Z_ITERABLE__
  void *bucket;
  void *node;
  void *data;
};

struct z_hash_map_plug {
  int     (*open)       (z_hash_map_t *self);
  void    (*close)      (z_hash_map_t *self);

  int     (*resize)     (z_hash_map_t *self,
                         unsigned int capacity);

  void    (*clear)      (z_hash_map_t *self);
  int     (*put)        (z_hash_map_t *self,
                         uint32_t hash,
                         void *key_value);
  void *  (*get)        (z_hash_map_t *self,
                         z_compare_t key_compare,
                         uint32_t hash,
                         const void *key);
  void *  (*pop)        (z_hash_map_t *self,
                         z_compare_t key_compare,
                         uint32_t hash,
                         const void *key);

  void *  (*iter_begin)  (z_hash_map_iterator_t *iter,
                          const z_hash_map_t *map);
  void *  (*iter_end)    (z_hash_map_iterator_t *iter,
                          const z_hash_map_t *map);
  void *  (*iter_next)   (z_hash_map_iterator_t *iter,
                          const z_hash_map_t *map);
  void *  (*iter_prev)   (z_hash_map_iterator_t *iter,
                          const z_hash_map_t *map);
};

struct z_hash_map {
  __Z_OBJECT__(z_map)

  const z_hash_map_plug_t *plug;

  z_mem_free_t data_free;
  z_hash_func_t hash;
  z_compare_t key_compare;
  void *udata;

  void *buckets;
  unsigned int capacity;
  unsigned int size;
  unsigned int seed;
};

extern const z_hash_map_plug_t z_open_hash_map;

z_hash_map_t *  z_hash_map_alloc          (z_hash_map_t *self,
                                           const z_hash_map_plug_t *plug,
                                           z_hash_func_t hash_func,
                                           z_compare_t key_compare,
                                           z_mem_free_t data_free,
                                           void *user_data,
                                           unsigned int seed,
                                           unsigned int capacity);
void            z_hash_map_free           (z_hash_map_t *self);

void            z_hash_map_clear          (z_hash_map_t *self);

int             z_hash_map_put            (z_hash_map_t *self,
                                           void *key_value);

void *          z_hash_map_get            (z_hash_map_t *self,
                                           const void *key);
void *          z_hash_map_get_custom     (z_hash_map_t *self,
                                           z_compare_t key_compare,
                                           uint32_t hash,
                                           const void *key);

void *          z_hash_map_pop            (z_hash_map_t *self,
                                           const void *key);
void *          z_hash_map_pop_custom     (z_hash_map_t *self,
                                           z_compare_t key_compare,
                                           uint32_t hash,
                                           const void *key);

int             z_hash_map_remove         (z_hash_map_t *self,
                                           const void *key);
int             z_hash_map_remove_custom  (z_hash_map_t *self,
                                           z_compare_t key_compare,
                                           uint32_t hash,
                                           const void *key);

#endif /* !_Z_HASH_MAP_H_ */
