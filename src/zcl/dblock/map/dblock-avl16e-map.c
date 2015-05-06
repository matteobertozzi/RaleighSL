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

#include "dblock-map-private.h"

#include <zcl/int-coding.h>
#include <zcl/memutil.h>
#include <zcl/dblock.h>
#include <zcl/avl16.h>

/*
 * +------+--------------+---------------+
 * | head |   data --->  |  <--- index   |
 * +------+--------------+---------------+
 */

typedef struct z_dblock_avl16e_map_head z_dblock_avl16e_map_head_t;
typedef struct z_dblock_avl16e_map_iter z_dblock_avl16e_map_iter_t;

struct z_dblock_avl16e_map_head {
  /* read vars */
  z_avl16_head_t avl_head;
  uint32_t blk_size;
  uint32_t kv_edge[2];
  /* write vars */
  uint32_t next_data;
  uint32_t index_blk;
  uint16_t index_next;
  uint16_t flags;
  uint32_t kv_last;
  z_dblock_kv_stats_t kv_stats;
} __attribute__((packed));

struct z_dblock_avl16e_map_iter {
  const uint8_t *block;
  z_avl16_iter_t idx_iter;
};

#define __dblock_avl16e_blk_used(head)                                    \
  ((head)->next_data + ((head)->kv_stats.kv_count << 3))

#define __dblock_avl16e_blk_avail(head)                                   \
  ((head)->blk_size - __dblock_avl16e_blk_used(head))

 static int __record_compare (void *udata,
                             const z_avl16_node_t *node,
                             const void *key)
{
  const z_dblock_kv_t *kv = Z_CONST_CAST(z_dblock_kv_t, key);
  uint8_t *block = Z_CAST(uint8_t, udata);
  z_dblock_kv_t rkv;
  uint32_t offset;
  z_uint32_decode(node->data, 3, &offset);
  z_dblock_log_map_record_get(block, block + offset, &rkv);
  return z_mem_compare(rkv.key, rkv.klength, kv->key, kv->klength);
}

static void __record_get (const z_dblock_avl16e_map_head_t *head,
                          const uint8_t *block,
                          const uint16_t node_pos,
                          z_dblock_kv_t *kv)
{
  z_avl16_node_t *node = Z_AVL16_NODE(block + head->index_blk, node_pos, 8);
  uint32_t offset;
  z_uint32_decode(node->data, 3, &offset);
  z_dblock_log_map_record_get(block, block + offset, kv);
}

/* ============================================================================
 *  Direct Lookup methods
 */
static int __dblock_avl16e_map_lookup (uint8_t *block, z_dblock_kv_t *kv) {
  z_dblock_avl16e_map_head_t *head = Z_CAST(z_dblock_avl16e_map_head_t, block);
  uint16_t node_pos;

  node_pos = z_avl16_lookup(block + head->index_blk, 8, head->avl_head.root,
                            __record_compare, kv, block);
  if (node_pos == 0)
    return(0);

  __record_get(head, block, node_pos, kv);
  return(1);
}

static void __dblock_avl16e_map_first_key (uint8_t *block, z_dblock_kv_t *kv) {
  const z_dblock_avl16e_map_head_t *head = Z_CONST_CAST(z_dblock_avl16e_map_head_t, block);
  __record_get(head, block, head->avl_head.edge[0], kv);
}

static void __dblock_avl16e_map_last_key (uint8_t *block, z_dblock_kv_t *kv) {
  const z_dblock_avl16e_map_head_t *head = Z_CONST_CAST(z_dblock_avl16e_map_head_t, block);
  __record_get(head, block, head->avl_head.edge[1], kv);
}

static void __dblock_avl16e_map_get_iptr (uint8_t *block, uint32_t iptr, z_dblock_kv_t *kv) {
  const z_dblock_avl16e_map_head_t *head = Z_CONST_CAST(z_dblock_avl16e_map_head_t, block);
  __record_get(head, block, iptr, kv);
}

/* ============================================================================
 *  Iterator methods
 */
static int __dblock_avl16e_map_seek (z_dblock_iter_t *viter,
                                    uint8_t *block,
                                    z_dblock_seek_t seek_pos,
                                    const z_dblock_kv_t *kv)
{
  const z_dblock_avl16e_map_head_t *head = Z_CONST_CAST(z_dblock_avl16e_map_head_t, block);
  z_dblock_avl16e_map_iter_t *iter = Z_CAST(z_dblock_avl16e_map_iter_t, viter->data);

  iter->block = block;
  z_avl16_iter_init(&(iter->idx_iter), block + head->index_blk, 8, head->avl_head.root);
  switch (seek_pos) {
    case Z_DBLOCK_SEEK_BEGIN:
      return(!!z_avl16_iter_seek_begin(&(iter->idx_iter)));
    case Z_DBLOCK_SEEK_END:
      return(!!z_avl16_iter_seek_end(&(iter->idx_iter)));
    case Z_DBLOCK_SEEK_LT:
      if (z_avl16_iter_seek_le(&(iter->idx_iter), __record_compare, kv, block) == NULL)
        return(0);
      return(iter->idx_iter.found ? z_avl16_iter_prev(&(iter->idx_iter)) != NULL : 1);
    case Z_DBLOCK_SEEK_LE:
      return(z_avl16_iter_seek_le(&(iter->idx_iter), __record_compare, kv, block) != NULL);
    case Z_DBLOCK_SEEK_GT:
      if (z_avl16_iter_seek_ge(&(iter->idx_iter), __record_compare, kv, block) == NULL)
        return(0);
      return(iter->idx_iter.found ? z_avl16_iter_next(&(iter->idx_iter)) != NULL : 1);
    case Z_DBLOCK_SEEK_GE:
      return(z_avl16_iter_seek_ge(&(iter->idx_iter), __record_compare, kv, block) != NULL);
    case Z_DBLOCK_SEEK_EQ:
      z_avl16_iter_seek_le(&(iter->idx_iter), __record_compare, kv, block);
      return(iter->idx_iter.found);
  }
  return(0);
}

static int __dblock_avl16e_map_seek_next (z_dblock_iter_t *viter) {
  z_dblock_avl16e_map_iter_t *iter = Z_CAST(z_dblock_avl16e_map_iter_t, viter->data);
  return(z_avl16_iter_next(&(iter->idx_iter)) != NULL);
}

static int __dblock_avl16e_map_seek_prev (z_dblock_iter_t *viter) {
  z_dblock_avl16e_map_iter_t *iter = Z_CAST(z_dblock_avl16e_map_iter_t, viter->data);
  return(z_avl16_iter_prev(&(iter->idx_iter)) != NULL);
}

static void __dblock_avl16e_map_seek_item (z_dblock_iter_t *viter, z_dblock_kv_t *kv) {
  z_dblock_avl16e_map_iter_t *iter = Z_CAST(z_dblock_avl16e_map_iter_t, viter->data);
  const z_dblock_avl16e_map_head_t *head = Z_CONST_CAST(z_dblock_avl16e_map_head_t, iter->block);
  __record_get(head, iter->block, iter->idx_iter.current, kv);
}

static uint32_t __dblock_avl16e_map_seek_iptr (z_dblock_iter_t *viter) {
  z_dblock_avl16e_map_iter_t *iter = Z_CAST(z_dblock_avl16e_map_iter_t, viter->data);
  return(iter->idx_iter.current);
}

/* ============================================================================
 *  Insert/Append/Prepend methods
 */
static uint32_t __dblock_avl16e_map_prepend (uint8_t *block, const z_dblock_kv_t *kv) {
  z_dblock_avl16e_map_head_t *head = Z_CAST(z_dblock_avl16e_map_head_t, block);
  z_avl16_node_t *node;
  uint8_t *pindex;
  uint32_t dpos;

  /* append record */
  dpos = head->next_data;
  z_dblock_log_map_record_add(block, &(head->next_data), &(head->kv_last), &(head->kv_stats), kv);

  /* append to the index */
  pindex = block + head->index_blk;
  z_avl16_prepend(&(head->avl_head), pindex, 8, head->index_next);
  node = Z_AVL16_NODE(pindex, head->index_next, 8);
  z_uint32_encode(node->data, 3, dpos);
  head->kv_edge[0] = head->index_next--;
  return(__dblock_avl16e_blk_avail(head));
}

static uint32_t __dblock_avl16e_map_append (uint8_t *block, const z_dblock_kv_t *kv) {
  z_dblock_avl16e_map_head_t *head = Z_CAST(z_dblock_avl16e_map_head_t, block);
  z_avl16_node_t *node;
  uint8_t *pindex;
  uint32_t dpos;

  /* append record */
  dpos = head->next_data;
  z_dblock_log_map_record_add(block, &(head->next_data), &(head->kv_last), &(head->kv_stats), kv);

  /* append to the index */
  pindex = block + head->index_blk;
  z_avl16_append(&(head->avl_head), pindex, 8, head->index_next);
  node = Z_AVL16_NODE(pindex, head->index_next, 8);
  z_uint32_encode(node->data, 3, dpos);
  head->kv_edge[1] = head->index_next--;
  return(__dblock_avl16e_blk_avail(head));
}

static uint32_t __dblock_avl16e_map_insert (uint8_t *block, const z_dblock_kv_t *kv) {
  z_dblock_avl16e_map_head_t *head = Z_CAST(z_dblock_avl16e_map_head_t, block);
  if (Z_LIKELY(head->kv_stats.kv_count > 0)) {
    z_avl16_node_t *node;
    z_dblock_kv_t ikv;
    uint8_t *pindex;
    uint32_t dpos;

    __dblock_avl16e_map_last_key(block, &ikv);
    if (z_mem_compare(kv->key, kv->klength, ikv.key, ikv.klength) > 0)
      return __dblock_avl16e_map_append(block, kv);

    __dblock_avl16e_map_first_key(block, &ikv);
    if (z_mem_compare(kv->key, kv->klength, ikv.key, ikv.klength) < 0)
      return __dblock_avl16e_map_prepend(block, kv);

    /* append record */
    dpos = head->next_data;
    z_dblock_log_map_record_add(block, &(head->next_data), &(head->kv_last), &(head->kv_stats), kv);

    /* insert in the index */
    pindex = block + head->index_blk;
    z_avl16_insert(&(head->avl_head), pindex, 8, head->index_next,
                   __record_compare, kv, block);
    node = Z_AVL16_NODE(pindex, head->index_next, 8);
    z_uint32_encode(node->data, 3, dpos);
    head->index_next--;
    return(__dblock_avl16e_blk_avail(head));
  }
  return __dblock_avl16e_map_append(block, kv);
}

static int __dblock_avl16e_map_replace (uint8_t *block,
                                        const z_dblock_kv_t *key,
                                        const z_dblock_kv_t *kv)
{
  z_dblock_avl16e_map_head_t *head = Z_CAST(z_dblock_avl16e_map_head_t, block);
  z_avl16_node_t *node;
  uint8_t *pindex;
  uint16_t npos;

  /* lookup and replace index */
  pindex = block + head->index_blk;
  npos = z_avl16_lookup(pindex, 8, head->avl_head.root, __record_compare, key, block);
  if (Z_UNLIKELY(npos == 0)) return(1);

  node = Z_AVL16_NODE(pindex, npos, 8);
  z_uint32_encode(node->data, 3, head->next_data);

  /* append record */
  z_dblock_log_map_record_add(block, &(head->next_data), &(head->kv_last), &(head->kv_stats), kv);
  return(0);
}

static void __dblock_avl16e_map_remove (uint8_t *block, const z_dblock_kv_t *kv) {
  z_dblock_avl16e_map_head_t *head = Z_CAST(z_dblock_avl16e_map_head_t, block);
  z_avl16_remove(&(head->avl_head), block + head->index_blk, 8,
                 __record_compare, kv, block);
}

/* ============================================================================
 *  Stats and utility methods
 */
static int __dblock_avl16e_map_has_space (uint8_t *block, const z_dblock_kv_t *kv) {
  const z_dblock_avl16e_map_head_t *head = Z_CONST_CAST(z_dblock_avl16e_map_head_t, block);
  return((8 + z_dblock_log_map_kv_space(kv)) <= (__dblock_avl16e_blk_avail(head)));
}

static uint32_t __dblock_avl16e_map_max_overhead (uint8_t *block) {
  return(8 + Z_DBLOCK_LOG_MAP_MAX_OVERHEAD);
}

static void __dblock_avl16e_map_stats (uint8_t *block, z_dblock_stats_t *stats) {
  const z_dblock_avl16e_map_head_t *head = Z_CONST_CAST(z_dblock_avl16e_map_head_t, block);
  stats->blk_size  = head->blk_size;
  stats->blk_avail = __dblock_avl16e_blk_avail(head);
  stats->is_sorted = 1;
  z_dblock_kv_stats_copy(&(stats->kv_stats), &(head->kv_stats));
}

static void __dblock_avl16e_map_init (uint8_t *block, const z_dblock_opts_t *opts) {
  z_dblock_avl16e_map_head_t *head = Z_CAST(z_dblock_avl16e_map_head_t, block);

  z_avl16_init(&(head->avl_head));

  head->blk_size   = opts->blk_size;
  head->kv_edge[0] = sizeof(z_dblock_avl16e_map_head_t);
  head->kv_edge[1] = sizeof(z_dblock_avl16e_map_head_t);

  if (head->blk_size >= (1 << 19)) {
    head->index_blk  = (head->blk_size - (1 << 19));
    head->index_next = 0xffff;
  } else {
    head->index_blk  = 0;
    head->index_next = (head->blk_size >> 3);
  }
  head->next_data  = sizeof(z_dblock_avl16e_map_head_t);

  head->kv_last    = 0;

  z_dblock_kv_stats_init(&(head->kv_stats));
}

const z_dblock_vtable_t z_dblock_avl16e_map = {
  .lookup       = __dblock_avl16e_map_lookup,
  .first_key    = __dblock_avl16e_map_first_key,
  .last_key     = __dblock_avl16e_map_last_key,
  .get_iptr     = __dblock_avl16e_map_get_iptr,

  .seek         = __dblock_avl16e_map_seek,
  .seek_next    = __dblock_avl16e_map_seek_next,
  .seek_prev    = __dblock_avl16e_map_seek_prev,
  .seek_item    = __dblock_avl16e_map_seek_item,
  .seek_iptr    = __dblock_avl16e_map_seek_iptr,

  .insert       = __dblock_avl16e_map_insert,
  .append       = __dblock_avl16e_map_append,
  .prepend      = __dblock_avl16e_map_prepend,
  .remove       = __dblock_avl16e_map_remove,
  .replace      = __dblock_avl16e_map_replace,

  .has_space    = __dblock_avl16e_map_has_space,
  .max_overhead = __dblock_avl16e_map_max_overhead,

  .stats        = __dblock_avl16e_map_stats,
  .init         = __dblock_avl16e_map_init,
};
