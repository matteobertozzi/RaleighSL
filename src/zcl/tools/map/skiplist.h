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

#ifndef _Z_SKIPLIST_H_
#define _Z_SKIPLIST_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/iterator.h>
#include <zcl/object.h>
#include <zcl/memory.h>
#include <zcl/map.h>

#define Z_SKIP_LIST_ITERATOR(x)         Z_CAST(z_skip_list_iterator_t, x)
#define Z_CONST_SKIP_LIST(x)            Z_CONST_CAST(z_skip_list_t, x)
#define Z_SKIP_LIST(x)                  Z_CAST(z_skip_list_t, x)

Z_TYPEDEF_STRUCT(z_skip_list_iterator)
Z_TYPEDEF_STRUCT(z_skip_list_node)
Z_TYPEDEF_STRUCT(z_skip_list)

struct z_skip_list_iterator {
  __Z_ITERABLE__
  z_skip_list_node_t *current;
};

struct z_skip_list_node {
  void *data;
  z_skip_list_node_t *next[0];
};

struct z_skip_list {
  __Z_OBJECT__(z_iterator)

  z_skip_list_node_t *head;
  z_mem_free_t data_free;
  z_compare_t key_compare;
  void *user_data;
  void *mutations;

  unsigned int levels;
  unsigned int seed;
  unsigned int size;
};

z_skip_list_node_t *z_skip_node_alloc       (unsigned int level,
                                             void *data);
void                z_skip_node_free        (z_skip_list_t *skip,
                                             z_skip_list_node_t *node);
unsigned int        z_skip_node_rand_level  (z_skip_list_t *skip);

z_skip_list_node_t *z_skip_node_lookup      (z_skip_list_t *skip,
                                             z_compare_t key_compare,
                                             const void *key);
z_skip_list_node_t *z_skip_node_less        (z_skip_list_t *skip,
                                             z_compare_t key_compare,
                                             const void *key);
z_skip_list_node_t *z_skip_node_less_eq     (z_skip_list_t *skip,
                                             z_compare_t key_compare,
                                             const void *key,
                                             int *found);

z_skip_list_t *     z_skip_list_alloc   (z_skip_list_t *self,
                                         z_compare_t key_compare,
                                         z_mem_free_t data_free,
                                         void *user_data,
                                         unsigned int seed);
void                z_skip_list_free    (z_skip_list_t *self);

void    z_skip_list_attach         (z_skip_list_t *self,
                                    unsigned int levels,
                                    z_skip_list_node_t *node);
void    z_skip_list_detach        (z_skip_list_t *self,
                                   z_skip_list_node_t *node);

void    z_skip_list_clear         (z_skip_list_t *self);

int     z_skip_list_put           (z_skip_list_t *self,
                                   void *key_value);

#define z_skip_list_remove(self, key)                                 \
  z_skip_list_remove_custom(self, (self)->key_compare, key)

int     z_skip_list_remove_custom (z_skip_list_t *self,
                                   z_compare_t key_compare,
                                   const void *key);

#define z_skip_list_get(self, key)                                    \
  z_skip_list_get_custom(self, (self)->key_compare, key)

void *  z_skip_list_get_custom    (z_skip_list_t *self,
                                   z_compare_t key_compare,
                                   const void *key);

__Z_END_DECLS__

#endif /* _Z_SKIPLIST_H_ */
