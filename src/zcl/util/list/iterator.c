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

#include <zcl/iterator.h>

/* ============================================================================
 *  PRIVATE Bytes Vector Iterator methods
 */
static int __bytes_vec_iter_begin (void *self) {
  z_bytes_vec_iter_t *iter = (z_bytes_vec_iter_t *)self;
  if (Z_LIKELY(iter->size > 0)) {
    iter->ihead.entry = iter->entries[iter->current++];
    return(1);
  }
  iter->ihead.entry = NULL;
  return(0);
}

static int __bytes_vec_iter_next (void *self) {
  z_bytes_vec_iter_t *iter = (z_bytes_vec_iter_t *)self;
  if (Z_LIKELY(iter->current < iter->size)) {
    iter->ihead.entry = iter->entries[iter->current++];
    return(1);
  }
  iter->ihead.entry = NULL;
  return(0);
}

static int __bytes_vec_iter_seek (void *self, unsigned int index) {
  z_bytes_vec_iter_t *iter = (z_bytes_vec_iter_t *)self;
  if (Z_LIKELY(index < iter->size)) {
    iter->ihead.entry = iter->entries[index];
    iter->current = index;
    return(1);
  }
  iter->ihead.entry = NULL;
  return(-1);
}

static const z_vtable_iterator_t __bytes_vec_iterator = {
  .begin = __bytes_vec_iter_begin,
  .next  = __bytes_vec_iter_next,
  .seek  = __bytes_vec_iter_seek,
};

/* ============================================================================
 *  PUBLIC Bytes Vector Iterator methods
 */
void z_bytes_vec_iter_open (z_bytes_vec_iter_t *self,
                            const z_memslice_t **entries,
                            unsigned int nentries)
{
  z_iterator_init(&(self->ihead), &__bytes_vec_iterator);
  self->entries = entries;
  self->current = 0;
  self->size = nentries;
}

/* ============================================================================
 *  PRIVATE Bytes Vector Iterator methods
 */
static int __fixed_vec_iter_begin (void *self) {
  z_fixed_vec_iter_t *iter = (z_fixed_vec_iter_t *)self;
  if (Z_LIKELY(iter->size > 0)) {
    z_memslice_t *entry = &(iter->ihead.entry_buf);
    iter->ihead.entry = entry;
    entry->data = Z_UINT8_PTR(iter->entries);
    return(1);
  }
  iter->ihead.entry = NULL;
  return(0);
}

static int __fixed_vec_iter_next (void *self) {
  z_fixed_vec_iter_t *iter = (z_fixed_vec_iter_t *)self;
  if (Z_LIKELY(iter->current < iter->size)) {
    z_memslice_t *entry = &(iter->ihead.entry_buf);
    iter->ihead.entry = entry;
    entry->data = Z_UINT8_PTR(iter->entries + (iter->current++ * iter->stride));
    return(1);
  }
  iter->ihead.entry = NULL;
  return(0);
}

static int __fixed_vec_iter_seek (void *self, unsigned int index) {
  z_fixed_vec_iter_t *iter = (z_fixed_vec_iter_t *)self;
  if (Z_LIKELY(index < iter->size)) {
    z_memslice_t *entry = &(iter->ihead.entry_buf);
    iter->ihead.entry = entry;
    entry->data = Z_UINT8_PTR(iter->entries + (index * iter->stride));
    iter->current = index;
    return(1);
  }
  iter->ihead.entry = NULL;
  return(-1);
}

static const z_vtable_iterator_t __fixed_vec_iterator = {
  .begin = __fixed_vec_iter_begin,
  .next  = __fixed_vec_iter_next,
  .seek  = __fixed_vec_iter_seek,
};

/* ============================================================================
 *  PUBLIC Fixed-Size Vector Iterator methods
 */
void z_fixed_vec_iter_open (z_fixed_vec_iter_t *self,
                            const void *entries,
                            unsigned int nentries,
                            unsigned int stride,
                            unsigned int width)
{
  z_iterator_init(&(self->ihead), &__fixed_vec_iterator);
  self->ihead.entry_buf.size = width;
  self->entries = Z_CONST_UINT8_PTR(entries);
  self->current = 0;
  self->size = nentries;
  self->stride = stride;
  self->width = width;
}