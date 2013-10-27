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
#include <zcl/btree.h>
#include <zcl/debug.h>

int z_btree_node_search (const uint8_t *node,
                         const z_vtable_btree_node_t *vtable,
                         const void *key, size_t ksize,
                         z_btree_item_t *item)
{
  const uint8_t *pkey = (const uint8_t *)key;
  uint32_t koffset = 0;
  int has_item;

  has_item = vtable->fetch_first(node, item);
  while (has_item) {
    uint32_t kshared;

    if (item->kprefix > koffset) {
      has_item = vtable->fetch_next(node, item);
      continue;
    }

    if (koffset > item->kprefix)
      return(1);

    kshared = z_memshared(item->key, item->ksize, pkey, ksize);
    if (kshared == ksize && item->ksize == ksize)
      return(item->is_deleted);

    if (item->key[kshared] > pkey[kshared])
      return(-1);

    koffset += kshared;
    ksize -= kshared;
    pkey += kshared;

    has_item = vtable->fetch_next(node, item);
  }

  return(1);
}

static int __node_item_cmp (z_btree_item_t *item_a, z_btree_item_t *item_b) {
  uint32_t kshared;
  int cmp;

  cmp = (item_b->kprefix - item_a->kprefix);
  if (cmp != 0)
    return(cmp);

  kshared = z_memshared(item_a->key, item_a->ksize, item_b->key, item_b->ksize);
  Z_ASSERT(!(kshared == item_a->ksize && item_a->ksize == item_b->ksize),
           "same keys are not allowed");

  if ((cmp = (item_a->key[kshared] - item_b->key[kshared])) < 0) {
    item_b->kprefix += kshared;
    item_b->ksize -= kshared;
    item_b->key += kshared;
  } else {
    item_a->kprefix += kshared;
    item_a->ksize -= kshared;
    item_a->key += kshared;
  }
  return(cmp);
}

void z_btree_node_merge (const uint8_t *node_a,
                         const z_vtable_btree_node_t *vtable_a,
                         const uint8_t *node_b,
                         const z_vtable_btree_node_t *vtable_b,
                         z_btree_item_func_t item_func,
                         void *udata)
{
  z_btree_item_t item_a;
  z_btree_item_t item_b;
  int has_a, has_b;
  int cmp;

  has_a = vtable_a->fetch_first(node_a, &item_a);
  has_b = vtable_b->fetch_first(node_b, &item_b);
  while (has_a && has_b) {
    if ((cmp = __node_item_cmp(&item_a, &item_b)) < 0) {
      item_func(udata, &item_a);
      has_a = vtable_a->fetch_next(node_a, &item_a);
    } else {
      item_func(udata, &item_b);
      has_b = vtable_b->fetch_next(node_b, &item_b);
    }
  }

  while (has_a) {
    item_func(udata, &item_a);
    has_a = vtable_a->fetch_next(node_a, &item_a);
  }

  while (has_b) {
    item_func(udata, &item_b);
    has_b = vtable_b->fetch_next(node_b, &item_b);
  }
}

struct multi_merge {
  uint8_t kbuffer[8][128];
  z_btree_item_t items[8];
  int has_data[8];
};

static int __merge_key_compare (struct multi_merge *merge, int a, int b) {
  z_btree_item_t *item_a = &(merge->items[a]);
  z_btree_item_t *item_b = &(merge->items[b]);
  int a_size = item_a->kprefix + item_a->ksize;
  int b_size = item_b->kprefix + item_b->ksize;
  int len = z_min(a_size, b_size);
  int cmp;
  if ((cmp = z_memcmp(merge->kbuffer[a], merge->kbuffer[b], len)) != 0)
    return(cmp);
  return((a_size - b_size));
}

static int __merge_fetch_next (struct multi_merge *merge,
                               const z_vtable_btree_node_t *vtable,
                               const uint8_t *node,
                               int index)
{
  z_btree_item_t *item = &(merge->items[index]);
  uint8_t *key = merge->kbuffer[index];
  do {
    merge->has_data[index] = vtable->fetch_next(node, item);
    z_memcpy(key + item->kprefix, item->key, item->ksize);
    key[item->kprefix + item->ksize] = 0;
    Z_LOG_TRACE("Fetch next %d", index);
  } while (item->is_deleted && merge->has_data[index]);
  return(merge->has_data[index]);
}

static void __merge_fetch_first (struct multi_merge *merge,
                                 const z_vtable_btree_node_t *vtable,
                                 const uint8_t *node,
                                 int index)
{
  z_btree_item_t *item = &(merge->items[index]);
  uint8_t *key = merge->kbuffer[index];
  merge->has_data[index] = vtable->fetch_first(node, item);
  z_memcpy(key, item->key, item->ksize);
  key[item->ksize] = 0;
  Z_LOG_TRACE("Fetch first %d", index);
  if (item->is_deleted && merge->has_data[index]) {
    __merge_fetch_next(merge, vtable, node, index);
  }
}

void z_btree_node_multi_merge (const uint8_t *nodes[],
                               unsigned int nnodes,
                               const z_vtable_btree_node_t *vtable,
                               z_btree_item_func_t item_func,
                               void *udata)
{
  struct multi_merge merge;
  z_btree_item_t item;
  int i, offset = 0;

  Z_ASSERT(nnodes <= 8, "Merging more than 8 nodes is not supported: %u", nnodes);

  for (i = 0; i < nnodes; ++i)
    __merge_fetch_first(&merge, vtable, nodes[i], i);

  for (i = 0; i < nnodes && !merge.has_data[i]; ++i)
    offset++;

  item.kprefix = 0;
  item.index = 0;
  item.is_deleted = 0;
  while (offset < nnodes) {
    int smallest = offset;
    int has_data = merge.has_data[offset];
    for (i = smallest; i < nnodes; ++i) {
      if (merge.has_data[i] && __merge_key_compare(&merge, i, smallest) < 0) {
        smallest = i;
        has_data = 1;
      }
    }

    if (!has_data)
      break;

    item.key   = merge.kbuffer[smallest];
    item.value = merge.items[smallest].value;
    item.ksize = merge.items[smallest].kprefix + merge.items[smallest].ksize;
    item.vsize = merge.items[smallest].vsize;
    item_func(udata, &item);

    if (!__merge_fetch_next(&merge, vtable, nodes[smallest], smallest)) {
      offset += (smallest == offset);
    }
  }
}
