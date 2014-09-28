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
#include <zcl/memutil.h>
#include <zcl/debug.h>
#include <zcl/math.h>

struct chain_head {
  uint32_t slots;
  uint32_t stride;
  uint32_t free_list;
  uint32_t avail;
} __attribute__((packed));

struct chain_slot {
  uint32_t hash;
  uint32_t next;
  uint8_t  data[1];
} __attribute__((packed));

#define __CHAIN_HEAD_SIZE         (sizeof(struct chain_head))

#define __slot_at(first_slot, stride, index)                              \
  Z_CAST(struct chain_slot, (first_slot) + ((index) * (stride)))

#define __slot_match(slot, khash, kcmp, key, udata)                       \
  ((slot)->hash == khash && !kcmp(udata, key, (slot)->data))

#define __first_bucket(block)                                             \
  Z_CAST(uint32_t, (block) + __CHAIN_HEAD_SIZE)

#define __first_entry(head, block)                                        \
  ((block) + __CHAIN_HEAD_SIZE + ((head)->slots << 2))

uint32_t z_chain32_map_init (uint8_t *block, uint32_t size,
                             uint16_t isize, uint16_t bucket_div)
{
  struct chain_head *head = (struct chain_head *)block;
  uint32_t max_items;
  uint32_t nbuckets;
  uint32_t stride;

  size = (size - __CHAIN_HEAD_SIZE);
  stride = sizeof(struct chain_slot) - 1 + isize;
  max_items = size / stride;

  /* adjust values */
  nbuckets = z_prime32(max_items / bucket_div);
  nbuckets = z_max(11, nbuckets);
  max_items = (size - (nbuckets << 2)) / stride;
  nbuckets += ((size - (max_items * stride) - (nbuckets << 2)) >> 2);
  Z_ASSERT(max_items > 0, "no space to fit the items block=%u stride",
           block + __CHAIN_HEAD_SIZE, stride);

  head->slots = nbuckets;
  head->stride = stride;
  head->free_list = 0;
  head->avail = max_items;
  memset(block + __CHAIN_HEAD_SIZE, 0, head->slots << 2);

  Z_DEBUG("block=%u slots=%u items=%u stride=%u",
          size + __CHAIN_HEAD_SIZE, head->slots, head->avail, head->stride);
  return(max_items);
}

void *z_chain32_map_get (uint8_t *block,
                         uint32_t hash,
                         z_compare_t key_cmp,
                         const void *key,
                         void *udata)
{
  const struct chain_head *head = (const struct chain_head *)block;
  const uint32_t *buckets;
  const uint8_t *entries;
  uint32_t index;

  buckets = __first_bucket(block);
  entries = __first_entry(head, block);

  Z_ASSERT(hash > 0, "hash must be greater than 0");
  index = buckets[hash % head->slots];
  while (index > 0) {
    struct chain_slot *slot = __slot_at(entries, head->stride, index - 1);
    if (__slot_match(slot, hash, key_cmp, key, udata)) {
      return(slot->data);
    }
    index = slot->next;
  }
  return(NULL);
}

void *z_chain32_map_put (uint8_t *block,
                         uint32_t hash,
                         z_compare_t key_cmp,
                         const void *key,
                         void *udata)
{
  struct chain_head *head = (struct chain_head *)block;
  struct chain_slot *slot;
  uint32_t *buckets;
  uint8_t *entries;
  uint32_t target;
  uint32_t index;

  buckets = __first_bucket(block);
  entries = __first_entry(head, block);

  Z_ASSERT(hash > 0, "hash must be greater than 0");
  target = hash % head->slots;
  index = buckets[target];
  while (index > 0) {
    slot = __slot_at(entries, head->stride, index - 1);
    if (__slot_match(slot, hash, key_cmp, key, udata)) {
      return slot->data;
    }
    index = slot->next;
  }

  if (head->free_list != 0) {
    index = head->free_list;
    slot = __slot_at(entries, head->stride, index - 1);
    head->free_list = slot->next;
  } else {
    if (head->avail == 0) {
      return(NULL);
    }
    index = head->avail--;
  }

  slot = __slot_at(entries, head->stride, index - 1);
  slot->hash = hash;
  slot->next = buckets[target];
  buckets[target] = index;
  return slot->data;
}

void *z_chain32_map_remove (uint8_t *block,
                            uint32_t hash,
                            z_compare_t key_cmp,
                            const void *key,
                            void *udata)
{
  struct chain_head *head = (struct chain_head *)block;
  struct chain_slot *last = NULL;
  uint32_t *buckets;
  uint8_t *entries;
  uint32_t target;
  uint32_t index;

  buckets = __first_bucket(block);
  entries = __first_entry(head, block);

  Z_ASSERT(hash > 0, "hash must be greater than 0");
  target = hash % head->slots;
  index = buckets[target];
  while (index > 0) {
    struct chain_slot *slot = __slot_at(entries, head->stride, index - 1);
    if (__slot_match(slot, hash, key_cmp, key, udata)) {
      if (last == NULL) {
        buckets[target] = slot->next;
      } else {
        last->next = slot->next;
      }
      slot->hash = 0;
      slot->next = head->free_list;
      head->avail++;
      head->free_list = index;
      return(slot->data);
    }
    last = slot;
    index = slot->next;
  }
  return(NULL);
}

