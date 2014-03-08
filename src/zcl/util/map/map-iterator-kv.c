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
 *  PRIVATE Map-Entry Iterator methods
 */
static int __map_kv_iter_begin (void *self) {
  z_map_kv_iter_t *iter = (z_map_kv_iter_t *)self;
  z_iterator_begin(iter->ikey);
  z_iterator_begin(iter->ivalue);
  if (Z_LIKELY(z_iterator_current(iter->ikey))) {
    z_map_entry_t *entry = &(iter->ihead.entry_buf);
    z_memslice_copy(&(entry->key), z_iterator_current(iter->ikey));
    z_memslice_copy(&(entry->value), z_iterator_current(iter->ivalue));
    iter->ihead.entry = entry;
  }
  iter->ihead.entry = NULL;
  return(0);
}

static int __map_kv_iter_next (void *self) {
  z_map_kv_iter_t *iter = (z_map_kv_iter_t *)self;
  z_iterator_begin(iter->ikey);
  z_iterator_begin(iter->ivalue);
  if (Z_LIKELY(z_iterator_current(iter->ikey))) {
    z_map_entry_t *entry = &(iter->ihead.entry_buf);
    z_memslice_copy(&(entry->key), z_iterator_current(iter->ikey));
    z_memslice_copy(&(entry->value), z_iterator_current(iter->ivalue));
    iter->ihead.entry = entry;
  }
  iter->ihead.entry = NULL;
  return(0);
}

static int __map_kv_iter_seek (void *self, const z_memslice_t *key) {
  z_map_kv_iter_t *iter = (z_map_kv_iter_t *)self;
  iter->ihead.entry = NULL;
  return(-1);
}

static const z_vtable_map_iterator_t __map_kv_iterator = {
  .begin = __map_kv_iter_begin,
  .next  = __map_kv_iter_next,
  .seek  = __map_kv_iter_seek,
};

/* ============================================================================
 *  PUBLIC Map-Entry Vector Iterator methods
 */
void z_map_kv_iter_open (z_map_kv_iter_t *self,
                         z_iterator_t *ikey,
                         z_iterator_t *ivalue)
{
  z_map_iterator_init(&(self->ihead), &__map_kv_iterator);
  self->ikey = ikey;
  self->ivalue = ivalue;
}
