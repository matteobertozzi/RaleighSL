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

#include <zcl/byteslice.h>
#include <zcl/string.h>
#include <zcl/debug.h>
#include <zcl/map.h>

void z_map_merger_open (z_map_merger_t *self) {
  z_dlink_init(&(self->merge_list));
  self->smallest_iter = NULL;
  self->skip_equals = 0;
}

void z_map_merger_close (z_map_merger_t *self) {
}

int z_map_merger_add (z_map_merger_t *self, z_map_iterator_t *iter) {
  const z_map_entry_t *entry = z_map_iterator_current(iter);
  if (Z_UNLIKELY(entry == NULL))
    return(1);

  z_dlink_add_tail(&(self->merge_list), &(iter->head.merge_list));
  return(0);
}

const z_map_entry_t *z_map_merger_next (z_map_merger_t *self) {
  const z_map_entry_t *smallest_entry = NULL;
  z_map_iterator_t *iter;

  if (Z_UNLIKELY(z_dlink_is_empty(&(self->merge_list))))
    return(NULL);

  if (self->smallest_iter != NULL) {
    if (!z_map_iterator_next(self->smallest_iter)) {
      z_dlink_del(&(self->smallest_iter->head.merge_list));
    }
    self->smallest_iter = NULL;
  }

  /* TODO: Optimize me */
  z_dlink_for_each_safe_entry(&(self->merge_list), iter, z_map_iterator_t, head.merge_list, {
    const z_map_entry_t *entry = z_map_iterator_current(iter);
    Z_ASSERT(entry != NULL, "NULL Entry for iterator %p", iter);

#if 0
    fprintf(stderr, "MERGER CMP %p: ", iter);
    z_dump_map_entry(stderr, entry);
#endif
    if (smallest_entry == NULL) {
      self->smallest_iter = iter;
      smallest_entry = entry;
    } else {
      int cmp = z_byte_slice_compare(&(entry->key), &(smallest_entry->key));
      if (cmp < 0) {
        self->smallest_iter = iter;
        smallest_entry = entry;
      } else if (cmp == 0 && self->skip_equals) {
        if (!z_map_iterator_next(iter)) {
          z_dlink_del(&(iter->head.merge_list));
        }
      }
    }
  });

  return(smallest_entry);
}
