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

#include <zcl/debug.h>
#include <zcl/map.h>

static int __memslice_compare (void *udata, const void *a, const void *b) {
  return(z_memslice_compare(Z_CONST_MEMSLICE(a), Z_CONST_MEMSLICE(b)));
}

void z_map_merger_init (z_map_merger_t *self, z_compare_t comparer) {
  z_dlink_init(&(self->merge_list));
  self->smallest_iter = NULL;
  self->key_comparer = (comparer != NULL) ? comparer : __memslice_compare;
  self->skip_equals = 0;
}

int z_map_merger_add (z_map_merger_t *self, z_map_iterator_t *iter) {
  if (Z_LIKELY(iter->entry != NULL)) {
    z_dlink_add_tail(&(self->merge_list), &(iter->merge_link));
    return(0);
  }
  return(1);
}

const z_map_entry_t *z_map_merger_next (z_map_merger_t *self) {
  const z_map_entry_t *smallest_entry = NULL;
  z_map_iterator_t *iter;

  if (self->smallest_iter != NULL) {
    if (!z_map_iterator_next(self->smallest_iter)) {
      z_dlink_del(&(self->smallest_iter->merge_link));
    }
    self->smallest_iter = NULL;
  }

  /* TODO: Optimize me */
  z_dlink_for_each_safe_entry(&(self->merge_list), iter, z_map_iterator_t, merge_link, {
    const z_map_entry_t *entry = iter->entry;
    Z_ASSERT(entry != NULL, "NULL Entry for iterator %p", iter);
    if (smallest_entry == NULL) {
      self->smallest_iter = iter;
      smallest_entry = entry;
    } else {
      int cmp = self->key_comparer(self, &(entry->key), &(smallest_entry->key));
      if (cmp < 0) {
        self->smallest_iter = iter;
        smallest_entry = entry;
      } else if (cmp == 0 && self->skip_equals) {
        if (!z_map_iterator_next(iter)) {
          z_dlink_del(&(iter->merge_link));
        }
      }
    }
  });

  return(smallest_entry);
}
#if 0
const z_map_entry_t *z_map_merger_search (z_map_merger_t *self,
                                          const void *key,
                                          uint32_t ksize)
{
  const z_map_entry_t *entry = NULL;
  z_map_iterator_t *iter;

  if (z_dlink_has_single_item(&(self->merge_list)) {
    iter = z_dlink_front_entry(&(self->merge_list), z_map_iterator_t, merge_link);
    entry = z_map_iterator_search(iter, key, ksize);
    self->smallest_iter = iter;
  } else {

  }

  return(entry);
}
#endif