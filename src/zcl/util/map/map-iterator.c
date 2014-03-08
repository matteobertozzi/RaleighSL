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

/* ============================================================================
 *  PRIVATE Map-Entry Vector Iterator methods
 */
static int __map_entry_vec_iter_begin (void *self) {
  z_map_entry_vec_iter_t *iter = (z_map_entry_vec_iter_t *)self;
  if (Z_LIKELY(iter->size > 0)) {
    iter->ihead.entry = iter->entries[iter->current++];
    return(1);
  }
  iter->ihead.entry = NULL;
  return(0);
}

static int __map_entry_vec_iter_next (void *self) {
  z_map_entry_vec_iter_t *iter = (z_map_entry_vec_iter_t *)self;
  if (Z_LIKELY(iter->current < iter->size)) {
    iter->ihead.entry = iter->entries[iter->current++];
    return(1);
  }
  iter->ihead.entry = NULL;
  return(0);
}

static int __map_entry_vec_iter_seek (void *self, const z_memslice_t *key) {
  z_map_entry_vec_iter_t *iter = (z_map_entry_vec_iter_t *)self;
  iter->ihead.entry = NULL;
  return(-1);
}

static const z_vtable_map_iterator_t __map_entry_vec_map_iterator = {
  .begin = __map_entry_vec_iter_begin,
  .next  = __map_entry_vec_iter_next,
  .seek  = __map_entry_vec_iter_seek,
};

/* ============================================================================
 *  PUBLIC Map-Entry Vector Iterator methods
 */
void z_map_entry_vec_iter_open (z_map_entry_vec_iter_t *self,
                                const z_map_entry_t **entries,
                                unsigned int nentries)
{
  z_map_iterator_init(&(self->ihead), &__map_entry_vec_map_iterator);
  self->entries = entries;
  self->current = 0;
  self->size = nentries;
}
