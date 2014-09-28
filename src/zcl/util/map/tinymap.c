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

struct tiny_head {
  uint16_t stride;
  uint16_t isize;
  uint32_t prime;
  uint32_t slots;
} __attribute__((packed));

struct tiny_slot {
  uint32_t hash;
  uint8_t  data[1];
} __attribute__((packed));

#define __tiny_HEAD_SIZE         sizeof(struct tiny_head)
#define __PRIME                  239

#define __slot_at(first_slot, stride, index)                              \
  Z_CAST(struct tiny_slot, (first_slot) + ((index) * (stride)))

#define __slot_match(slot, khash, kcmp, key, udata)                       \
  ((slot)->hash == khash && !kcmp(udata, key, (slot)->data))

#define __entry_probe(head, block, slot, hash, __code__)                  \
  do {                                                                    \
    const uint32_t incr = 1 + ((hash * __PRIME) % (head->prime - 1));     \
    const uint8_t *first_slot = block + __tiny_HEAD_SIZE;                 \
    register uint32_t i, islot;                                           \
                                                                          \
    islot = hash % head->slots;                                           \
    for (i = (islot < head->prime); i < head->prime; ++i) {               \
      slot = __slot_at(first_slot, head->stride, islot);                  \
      do __code__ while (0);                                              \
      islot = (islot + incr) % head->prime;                               \
    }                                                                     \
                                                                          \
    /* look into the remaining slots */                                   \
    for (islot = head->prime; islot < head->slots; ++islot) {             \
      slot = __slot_at(first_slot, head->stride, islot);                  \
      do __code__ while (0);                                              \
    }                                                                     \
  } while (0)


uint32_t z_tiny_map_init (uint8_t *block, uint32_t size, uint16_t isize) {
  struct tiny_head *head = (struct tiny_head *)block;
  memset(block, 0, size);
  head->stride = sizeof(struct tiny_slot) - 1 + isize;
  head->isize = isize;
  head->slots = (size - sizeof(struct tiny_head)) / head->stride;
  head->prime = z_lprime32(head->slots);
  Z_DEBUG("block=%u slots=%u prime=%u stride=%u isize=%u",
          size, head->slots, head->prime, head->stride, head->isize);
  return head->slots;
}

void *z_tiny_map_get (uint8_t *block,
                       uint32_t hash,
                       z_compare_t key_cmp,
                       const void *key,
                       void *udata)
{
  const struct tiny_head *head = (struct tiny_head *)block;
  struct tiny_slot *slot;

  Z_ASSERT(hash > 0, "hash must be greater than 0");
  __entry_probe(head, block, slot, hash, {
    if (__slot_match(slot, hash, key_cmp, key, udata)) {
      return(slot->data);
    }
  });
  return(NULL);
}

void *z_tiny_map_put (uint8_t *block,
                       uint32_t hash,
                       z_compare_t key_cmp,
                       const void *key,
                       void *udata)
{
  struct tiny_head *head = (struct tiny_head *)block;
  struct tiny_slot *slot;

  Z_ASSERT(hash > 0, "hash must be greater than 0");
  __entry_probe(head, block, slot, hash, {
    if (slot->hash == 0 || __slot_match(slot, hash, key_cmp, key, udata)) {
      slot->hash = hash;
      return(slot->data);
    }
  });
  return(NULL);
}

void *z_tiny_map_remove (uint8_t *block,
                          uint32_t hash,
                          z_compare_t key_cmp,
                          const void *key,
                          void *udata)
{
  struct tiny_head *head = (struct tiny_head *)block;
  struct tiny_slot *slot;

  Z_ASSERT(hash > 0, "hash must be greater than 0");
  __entry_probe(head, block, slot, hash, {
    if (__slot_match(slot, hash, key_cmp, key, udata)) {
      slot->hash = 0;
      return(slot->data);
    }
  });
  return(NULL);
}
