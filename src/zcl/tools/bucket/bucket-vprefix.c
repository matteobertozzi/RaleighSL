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

#include <zcl/checksum.h>
#include <zcl/coding.h>
#include <zcl/string.h>
#include <zcl/bucket.h>
#include <zcl/debug.h>

/*
 * btree node for key/values with variable size.
 * keys are written using prefix compression.
 *
 * +------+----+----+----+----+----------+----+----+----+----+
 * | head | k0 | k1 | .. | kN | -->  <-- | vN | .. | v1 | v0 |
 * +------+----+----+----+----+----------+----+----+----+----+
 *            /      \
 *  +--------+--------+--------------------+
 *  | head | kprefix | ksize | vsize | key |
 *  +--------------------------------------+
 *    3-int group encoding 3+ bytes
 */

/* ============================================================================
 *  PRIVATE node macros
 */
#define __CONST_NODE_HEAD(x)      Z_CONST_CAST(struct node_head, x)
#define __NODE_HEAD(x)            Z_CAST(struct node_head, x)

#define __node_space_avail(node)                                    \
  (__CONST_NODE_HEAD(node)->nh_last_value - __CONST_NODE_HEAD(node)->nh_last_key)

#define __node_first_key(node)                                      \
  ((node) + sizeof(struct node_head))

#define __node_last_key(node)                                       \
  ((node) + __CONST_NODE_HEAD(node)->nh_last_key)

/* ============================================================================
 *  PRIVATE node structs
 */

/* Node Header - 32 byte */
struct node_head {
  uint32_t nh_crc;          /* Node CRC - This entry must be the first one */
  uint32_t nh_size;

  uint32_t nh_last_key;     /* Offset of to the end of the last key */
  uint32_t nh_last_value;   /* Offset to the begin of the last value */

  uint16_t nh_items;
  uint8_t  nh_level;
  uint8_t  nh_prefix;

  uint32_t nh_ptrs[3];      /* 0 |   I   I   I   | N */
} __attribute__((packed));

/* ============================================================================
 *  PRIVATE node methods
 */

static uint32_t __node_checksum (const uint8_t *node, uint32_t size) {
  return(z_csum32_crcc(0, node + sizeof(uint32_t), size - sizeof(uint32_t)));
}

/* ============================================================================
 *  PUBLIC vtable node methods
 */
static void __node_create (uint8_t *node, uint32_t size) {
  struct node_head *head = (struct node_head *)node;

  z_memzero(node, size);
  head->nh_size = size;
  head->nh_last_key = sizeof(struct node_head);
  head->nh_last_value = size;
}

static int __node_open (const uint8_t *node, uint32_t size) {
  struct node_head *head = (struct node_head *)node;
  uint32_t crc;

  if (head->nh_size != size) {
    Z_LOG_WARN("Failed size verification - Expected %"PRIu32" got %"PRIu32,
               head->nh_size, size);
    return(1);
  }

  crc = __node_checksum(node, size);
  if (head->nh_crc != crc) {
    Z_LOG_WARN("Failed checksum verification - Expected %"PRIu32" got %"PRIu32,
               head->nh_crc, crc);
    return(2);
  }
  return(0);
}

static void __node_finalize (uint8_t *node) {
  struct node_head *head = (struct node_head *)node;
  head->nh_crc = __node_checksum(node, head->nh_size);
  Z_LOG_TRACE("Node Finalized size=%"PRIu32" with nh_items=%"PRIu32" use-prefix %d\n",
              head->nh_size, head->nh_items, head->nh_prefix);
}

static int __node_item_fetch (const uint8_t *node,
                              const uint8_t *pkey,
                              z_bucket_entry_t *item)
{
  uint8_t length[3];

  if (Z_UNLIKELY(pkey >= __node_last_key(node)))
    return(0);

  /* Decode key head */
  length[0] = z_fetch_3bit(*pkey, 5);           /* key prefix */
  length[1] = 1 + z_fetch_2bit(*pkey, 3);       /* key size   */
  length[2] = 1 + z_fetch_2bit(*pkey, 1);       /* value size */
  item->is_deleted = z_fetch_1bit(*pkey, 0);    /* is deleted */
  ++pkey;

  /* Decode key prefix, key size, value size */
  z_decode_uint32(pkey, length[0], &(item->kprefix));    pkey += length[0];
  z_decode_uint32(pkey, length[1], &(item->key.size));   pkey += length[1];
  z_decode_uint32(pkey, length[2], &(item->value.size)); pkey += length[2];

  item->key.data = (uint8_t *)pkey;
  item->value.data -= item->value.size;
  ++(item->index);
  return(1);
}

static uint32_t __node_calc_khead (const z_bucket_entry_t *item, uint8_t head[3]) {
  head[0] = (item->kprefix > 0) ? z_uint32_size(item->kprefix) : 0;
  head[1] = z_uint32_size(item->key.size);
  head[2] = z_uint32_size(item->value.size);
  return(1 + head[0] + head[1] + head[2]);
}

static uint32_t __node_available (const uint8_t *node) {
  return(__node_space_avail(node));
}

static int __node_has_space (const uint8_t *node, const z_bucket_entry_t *item) {
  uint8_t length[3];
  size_t required;
  required = __node_calc_khead(item, length) + item->key.size + item->value.size;
  return(__node_space_avail(node) >= required);
}

static int __node_append (uint8_t *node, const z_bucket_entry_t *item) {
  struct node_head *head = __NODE_HEAD(node);
  uint8_t *pkey = __node_last_key(node);
  uint8_t length[3];

  uint32_t ksize;

  ksize = item->key.size + __node_calc_khead(item, length);
  if (Z_UNLIKELY((ksize + item->value.size) > __node_space_avail(node)))
    return(1);

  head->nh_last_key += ksize;
  head->nh_last_value -= item->value.size;
  head->nh_prefix |= (item->kprefix > 0);
  ++(head->nh_items);

  /* Write value */
  z_memcpy(node + head->nh_last_value, item->value.data, item->value.size);

  /* Write key */
  *pkey++ = (length[0] << 5) | ((length[1] - 1) << 3) | ((length[2] - 1) << 1);
  z_encode_uint(pkey, length[0], item->kprefix);    pkey += length[0];
  z_encode_uint(pkey, length[1], item->key.size);   pkey += length[1];
  z_encode_uint(pkey, length[2], item->value.size); pkey += length[2];
  z_memcpy(pkey, item->key.data, item->key.size);
  return(0);
}

static void __node_remove (uint8_t *node, z_bucket_entry_t *item) {
  uint8_t length[3];
  uint8_t *pbuf;

  pbuf = (uint8_t *)(item->key.data) - __node_calc_khead(item, length);
  z_set_1bit(*pbuf, 0);
}

static int __node_fetch_first (const uint8_t *node, z_bucket_entry_t *item) {
  item->value.data = (uint8_t *)node + __CONST_NODE_HEAD(node)->nh_size;
  item->index = 0;
  return(__node_item_fetch(node, __node_first_key(node), item));
}

static int __node_fetch_next (const uint8_t *node, z_bucket_entry_t *item) {
  return(__node_item_fetch(node, item->key.data + item->key.size, item));
}

static void __node_fetch_first_key (const uint8_t *node, z_byte_slice_t *key) {
  z_bucket_entry_t entry;
  __node_fetch_first(node, &entry);
  z_byte_slice_copy(key, &(entry.key));
}

static int __node_is_prefix_encoded (const uint8_t *node) {
  return(__CONST_NODE_HEAD(node)->nh_prefix);
}

const z_vtable_bucket_t z_bucket_vprefix = {
  .create         = __node_create,
  .open           = __node_open,
  .finalize       = __node_finalize,

  .available      = __node_available,
  .has_space      = __node_has_space,
  .append         = __node_append,
  .remove         = __node_remove,

  .first_key      = __node_fetch_first_key,
  .fetch_first    = __node_fetch_first,
  .fetch_next     = __node_fetch_next,

  .prefix_encoded = __node_is_prefix_encoded,
};
