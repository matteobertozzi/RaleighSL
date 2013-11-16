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

#include <zcl/string.h>
#include <zcl/bucket.h>
#include <zcl/bytes.h>
#include <zcl/debug.h>

/* ============================================================================
 *  PUBLIC bucket search methods
 */
static int __bucket_search (const z_vtable_bucket_t *vtable,
                            const uint8_t *node,
                            const z_byte_slice_t *key,
                            z_bucket_entry_t *entry)
{
  const uint8_t *pkey = key->data;
  size_t ksize = key->size;
  uint32_t koffset = 0;
  int has_item;

  has_item = vtable->fetch_first(node, entry);
  while (has_item) {
    size_t kshared;

    if (entry->kprefix > koffset) {
      has_item = vtable->fetch_next(node, entry);
      continue;
    }

    if (koffset > entry->kprefix)
      return(1);

    kshared = z_memshared(entry->key.data, entry->key.size, pkey, ksize);
    if (kshared == ksize && entry->key.size == ksize)
      return(entry->is_deleted);

    if (entry->key.data[kshared] > pkey[kshared])
      return(-1);

    koffset += kshared;
    ksize -= kshared;
    pkey += kshared;

    has_item = vtable->fetch_next(node, entry);
  }

  return(1);
}

int z_bucket_search (const z_vtable_bucket_t *vtable,
                     const uint8_t *node,
                     const z_byte_slice_t *key,
                     z_byte_slice_t *value)
{
  z_bucket_entry_t entry;
  int cmp;
  cmp = __bucket_search(vtable, node, key, &entry);
  z_byte_slice_copy(value, &(entry.value));
  return(cmp);
}

/* ============================================================================
 *  PRIVATE bucket iterator methods
 */
static int __bucket_iter_next (void *self) {
  z_bucket_iterator_t *iter = Z_BUCKET_ITERATOR(self);
  uint32_t kprev_prefix;
  z_byte_slice_t kprev;
  do {
    z_byte_slice_copy(&kprev, &(iter->entry.key));
    kprev_prefix = iter->entry.kprefix;
    iter->has_data = iter->vtable->fetch_next(iter->node, &(iter->entry));
    if (iter->has_data && iter->entry.kprefix > 0) {
      z_memcpy(iter->kbuffer + kprev_prefix, kprev.data, iter->entry.kprefix);
      z_memcpy(iter->kbuffer + iter->entry.kprefix,
               iter->entry.key.data, iter->entry.key.size);
      z_byte_slice_set(&(iter->map_entry.key), iter->kbuffer,
                       iter->entry.kprefix + iter->entry.key.size);
    } else {
      z_byte_slice_copy(&(iter->map_entry.key), &(iter->entry.key));
    }
    z_byte_slice_copy(&(iter->map_entry.value), &(iter->entry.value));
  } while (iter->has_data && iter->entry.is_deleted);
  return(iter->has_data);
}

static int __bucket_iter_begin (void *self) {
  z_bucket_iterator_t *iter = Z_BUCKET_ITERATOR(self);
  iter->has_data = iter->vtable->fetch_first(iter->node, &(iter->entry));
  if (iter->has_data && iter->entry.is_deleted) {
    return(__bucket_iter_next(self));
  } else {
    z_byte_slice_copy(&(iter->map_entry.key), &(iter->entry.key));
    z_byte_slice_copy(&(iter->map_entry.value), &(iter->entry.value));
  }
  return(iter->has_data);
}

static const z_map_entry_t *__bucket_iter_current (const void *self) {
  const z_bucket_iterator_t *iter = Z_CONST_BUCKET_ITERATOR(self);
  return(iter->has_data ? &(iter->map_entry) : NULL);
}

static void __bucket_iter_get_refs (const void *self,
                                    z_bytes_ref_t *key,
                                    z_bytes_ref_t *value)
{
  const z_bucket_iterator_t *iter = Z_CONST_BUCKET_ITERATOR(self);
  if (Z_UNLIKELY(!iter->has_data))
    return;

  if (key != NULL) {
    if (iter->entry.kprefix > 0) {
      z_bytes_t *entry_key = z_bytes_from_byte_slice(&(iter->map_entry.key));
      z_bytes_ref_set(key, &(entry_key->slice), &z_vtable_bytes_refs, entry_key);
    } else {
      const z_map_iterator_head_t *ihead = &(Z_MAP_ITERATOR_HEAD(self));
      ihead->vrefs->inc_ref(ihead->object);
      z_bytes_ref_set(key, &(iter->map_entry.key), ihead->vrefs, ihead->object);
    }
  }

  if (value != NULL) {
    const z_map_iterator_head_t *ihead = &(Z_MAP_ITERATOR_HEAD(self));
    ihead->vrefs->inc_ref(ihead->object);
    z_bytes_ref_set(value, &(iter->map_entry.value), ihead->vrefs, ihead->object);
  }
}

static int __bucket_iter_seek (void *self, const z_byte_slice_t *key) {
  const z_map_entry_t *entry;
  int has_data;
  int cmp = 1;

  has_data = __bucket_iter_begin(self);
  while (has_data) {
    entry = __bucket_iter_current(self);
    cmp = z_byte_slice_compare(&(entry->key), key);
    if (cmp >= 0)
      break;

    has_data = __bucket_iter_next(self);
  }

  return(cmp);
}

void z_bucket_iterator_open (z_bucket_iterator_t *self,
                             const z_vtable_bucket_t *vtable,
                             const uint8_t *node,
                             const z_vtable_refs_t *vrefs,
                             void *object)
{
  Z_MAP_ITERATOR_INIT(self, &z_bucket_map_iterator, vrefs, object);
  self->vtable = vtable;
  self->node = node;
  self->has_data = 0;
}

const z_vtable_map_iterator_t z_bucket_map_iterator = {
  .begin    = __bucket_iter_begin,
  .next     = __bucket_iter_next,
  .current  = __bucket_iter_current,
  .get_refs = __bucket_iter_get_refs,
  .seek     = __bucket_iter_seek,
};
