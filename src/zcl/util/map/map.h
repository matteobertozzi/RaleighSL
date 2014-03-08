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

#include <zcl/memslice.h>
#include <zcl/comparer.h>
#include <zcl/iterator.h>
#include <zcl/macros.h>
#include <zcl/vtable.h>
#include <zcl/dlink.h>

Z_TYPEDEF_STRUCT(z_vtable_map_iterator)
Z_TYPEDEF_STRUCT(z_map_entry_vec_iter)
Z_TYPEDEF_STRUCT(z_map_kv_iter)
Z_TYPEDEF_STRUCT(z_map_iterator)
Z_TYPEDEF_STRUCT(z_map_merger)
Z_TYPEDEF_STRUCT(z_map_entry)

struct z_vtable_map_iterator {
  int   (*begin)  (void *self);
  int   (*next)   (void *self);
  int   (*seek)   (void *self, const z_memslice_t *key);
};

struct z_map_entry {
  z_memslice_t key;
  z_memslice_t value;
};

struct z_map_merger {
  z_dlink_node_t merge_list;
  z_map_iterator_t *smallest_iter;
  z_compare_t key_comparer;
  int skip_equals;
  int pad;
};

struct z_map_iterator {
  const z_vtable_map_iterator_t *vtable;
  z_dlink_node_t merge_link;
  z_map_entry_t entry_buf;
  const z_map_entry_t *entry;
};

struct z_map_kv_iter {
  z_map_iterator_t ihead;
  z_iterator_t *ikey;
  z_iterator_t *ivalue;
};

struct z_map_entry_vec_iter {
  z_map_iterator_t ihead;
  unsigned int current;
  unsigned int size;
  const z_map_entry_t **entries;
};

#define z_map_iterator_init(self, iter_vtable)        \
  do {                                                \
    (self)->vtable = iter_vtable;                     \
    z_dlink_init(&((self)->merge_link));              \
    (self)->entry = NULL;                             \
  } while (0)

#define z_map_iterator_begin(self)        z_vtable_call(self, begin)
#define z_map_iterator_next(self)         z_vtable_call(self, next)
#define z_map_iterator_seek(self, key)    z_vtable_call(self, seek, key)
#define z_map_iterator_current(self)      (self)->entry

void z_map_entry_vec_iter_open (z_map_entry_vec_iter_t *self,
                                const z_map_entry_t **entries,
                                unsigned int nentries);

void  z_map_merger_init (z_map_merger_t *self, z_compare_t comparer);
int   z_map_merger_add  (z_map_merger_t *self, z_map_iterator_t *iter);
const z_map_entry_t *z_map_merger_next (z_map_merger_t *self);

__Z_END_DECLS__

#endif /* _Z_MAP_H_ */
