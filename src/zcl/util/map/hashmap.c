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

#include <zcl/hashmap.h>
#include <zcl/debug.h>

struct map_slot {
  uint32_t hash;
  uint32_t next;
  uint8_t  data[1];
};

#define __BUCKETS_FANOUT      8
#define __ENTRIES_FANOUT      16

static void __hash_map_resize (z_hash_map_t *self) {
  z_array_resize(&(self->entries), 1 + z_array_length(&(self->entries)));

  /* resize the bucket-table if necessary (this is slow) */
  if ((self->items >> 2) >= z_array_length(&(self->buckets))) {
    z_array_t *buckets = &(self->buckets);
    z_array_iter_t iter;
    uint32_t index = 0;
    uint32_t items;

    //printf("RESIZE %u vs %u\n", self->items, z_array_length(&(self->buckets)));
    z_array_resize(buckets, 1 + z_array_length(buckets));
    z_array_zero_all(buckets);

    /* TODO: Insead of iterating the entries, iter the buckets */
    items = self->items;
    const uint32_t buckets_length = z_array_length(buckets);
    z_array_iter_init(&iter, &(self->entries));
    do {
      const uint32_t isize = self->entries.isize;
      uint32_t length = iter.length;
      uint8_t *pblob = iter.blob;

      while (length-- && items > 0) {
        struct map_slot *pslot = (struct map_slot *)pblob;

        index++;
        if (Z_LIKELY(pslot->hash > 0)) {
          const uint32_t target = pslot->hash % buckets_length;
          uint32_t *bucket;

          bucket = z_array_get(buckets, target, uint32_t);
          pslot->next = *bucket;
          *bucket = index;

          items--;
        }

        pblob += isize;
      }
    } while (z_array_iter_next(&iter));
  }
}

int z_hash_map_open (z_hash_map_t *self,
                     uint16_t isize,
                     uint32_t capacity,
                     uint32_t bucket_pagesz,
                     uint32_t entries_pagesz)
{
  int errnum;

  capacity = z_align_up(capacity, 64);

  errnum = z_array_open(&(self->buckets), 4,
                        __BUCKETS_FANOUT, bucket_pagesz, capacity);
  if (Z_UNLIKELY(errnum))
    return(errnum);

  errnum = z_array_open(&(self->entries), isize + 8,
                        __ENTRIES_FANOUT, entries_pagesz, capacity);
  if (Z_UNLIKELY(errnum)) {
    z_array_close(&(self->buckets));
    return(errnum);
  }

  self->items = 0;
  self->free_list = 0;

  z_array_zero_all(&(self->buckets));
  return(0);
}

void z_hash_map_close (z_hash_map_t *self) {
  z_array_close(&(self->entries));
  z_array_close(&(self->buckets));
}

void *z_hash_map_get (const z_hash_map_t *self,
                      uint32_t hash,
                      z_compare_t key_cmp,
                      const void *key,
                      void *udata)
{
  const z_array_t *buckets = &(self->buckets);
  const z_array_t *entries = &(self->entries);
  uint32_t index;

  index = z_array_get_u32(buckets, hash % z_array_length(buckets));
  while (index > 0) {
    struct map_slot *slot = z_array_get(entries, index - 1, struct map_slot);
    if (hash == slot->hash && !key_cmp(udata, key, slot->data)) {
      return(slot->data);
    }
    index = slot->next;
  }
  return(NULL);
}

void *z_hash_map_put (z_hash_map_t *self,
                      uint32_t hash,
                      z_compare_t key_cmp,
                      const void *key,
                      void *udata)
{
  z_array_t *buckets = &(self->buckets);
  z_array_t *entries = &(self->entries);
  struct map_slot *slot;
  uint32_t target;
  uint32_t index;

  Z_ASSERT(hash > 0, "hash must be greater than 0");
  target = hash % z_array_length(buckets);
  index = z_array_get_u32(buckets, target);
  while (index > 0) {
    slot = z_array_get(entries, index - 1, struct map_slot);
    if (hash == slot->hash && !key_cmp(udata, key, slot->data)) {
      return slot->data;
    }
    index = slot->next;
  }

  if (self->free_list != 0) {
    index = self->free_list - 1;
    slot = z_array_get(entries, index, struct map_slot);
    self->free_list = slot->next;
  } else {
    if (self->items == z_array_length(entries)) {
      __hash_map_resize(self);
      target = hash % z_array_length(buckets);
    }
    index = self->items++;
  }

  slot = z_array_get(entries, index, struct map_slot);
  slot->hash = hash;
  slot->next = z_array_get_u32(buckets, target);

  index++;
  z_array_set_ptr(buckets, target, &index);
  return slot->data;
}

void *z_hash_map_remove (z_hash_map_t *self,
                         uint32_t hash,
                         z_compare_t key_cmp,
                         const void *key,
                         void *udata)
{
  z_array_t *buckets = &(self->buckets);
  z_array_t *entries = &(self->entries);
  struct map_slot *last = NULL;
  uint32_t target;
  uint32_t index;

  target = hash % z_array_length(buckets);
  index = z_array_get_u32(buckets, target);
  while (index > 0) {
    struct map_slot *slot = z_array_get(entries, index - 1, struct map_slot);
    if (hash == slot->hash && !key_cmp(udata, key, slot->data)) {
      if (last == NULL) {
        z_array_set_ptr(buckets, target, &(slot->next));
      } else {
        last->next = slot->next;
      }
      slot->hash = 0;
      slot->next = self->free_list;
      self->items--;
      self->free_list = index;
      return(slot->data);
    }
    last = slot;
    index = slot->next;
  }
  return(NULL);
}