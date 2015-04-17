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

#include <zcl/dblock-map.h>
#include <zcl/int-coding.h>
#include <zcl/memutil.h>
#include <zcl/avl16.h>

typedef struct z_dblock_avl16_map_head z_dblock_avl16_map_head_t;

struct z_dblock_avl16_map_head {
  uint8_t format;
  uint8_t _pad[3];

  z_avl16_head_t avl_head;
  uint32_t blk_size;
  uint32_t blk_avail;
  uint32_t kv_count;
  /* write vars */
  uint32_t index_next;
  z_dblock_kv_stats_t kv_stats;
} __attribute__((packed));

static int __record_compare (void *udata,
                             const z_avl16_node_t *node,
                             const void *key)
{
  const z_dblock_kv_t *kv = Z_CONST_CAST(z_dblock_kv_t, key);
  const uint8_t *pbuf = node->data + 1;
  const uint8_t head = node->data[0];
  uint32_t klength;
  switch (head & 0xC0) {
    case 1 << 6: /* 01 KK VV VV -> klen=3  vlen=15 */
      klength = 1 + ((head & 0x30) >> 4);
      break;
    case 2 << 6: /* 10 KK KV VV -> klen=7  vlen=7 */
      klength = 1 + ((head & 0x38) >> 3);
      break;
    case 3 << 6: /* 11 KK KK KK -> klen=63 vlen=0 */
      klength = 1 + (head & 0x3f);
      break;
    case 0 << 6: { /* 00 00 KK VV -> klen=N vlen=N */
      const uint8_t ksize = (head & 0x0c) >> 2;
      const uint8_t vsize = (head & 0x03);
      z_uint32_decode(pbuf, ksize, &klength);
      pbuf += ksize + vsize;
      break;
    }
  }
  return z_mem_compare(pbuf, klength, kv->key, kv->klength);
}

static uint32_t __record_add (z_dblock_avl16_map_head_t *head,
                              uint8_t *block,
                              const z_dblock_kv_t *kv)
{
  z_avl16_node_t *node = Z_AVL16_NODE(block, head->index_next);
  uint32_t uspace;
  uint8_t *rhead;
  uint8_t *pbuf;

  rhead = node->data;
  pbuf  = node->data + 1;

  /*
   * 00 00 KK VV -> klen=N  vlen=N
   * 00 1K KK VV -> klen=7  vlen=N
   * 01 KK VV VV -> klen=3  vlen=15
   * 10 KK KV VV -> klen=7  vlen=7
   * 11 KK KK KK -> klen=63 vlen=0
   */
  if (((kv->klength - 1) + kv->vlength) <= 18) {
    if (kv->klength <= 4) {
      *rhead = 0x40 | (kv->klength - 1) << 4 | kv->vlength;
    } else {
      *rhead = 0x80 | (kv->klength - 1) << 3 | kv->vlength;
    }
  } else if (kv->vlength == 0 && kv->klength <= 64) {
    *rhead = 0xC0 | (kv->klength - 1);
  } else {
    const uint8_t vsize = z_uint32_size(kv->vlength);
    if (kv->klength <= 8) {
      *rhead = 0x20 | (kv->klength - 1) << 2 | vsize;
    } else {
      const uint8_t ksize = z_uint32_size(kv->klength);
      *rhead = (ksize << 2) | vsize;
      z_uint32_encode(pbuf, ksize, kv->klength); pbuf += ksize;
    }
    z_uint32_encode(pbuf, vsize, kv->vlength); pbuf += vsize;
  }

  memcpy(pbuf, kv->key,   kv->klength); pbuf += kv->klength;
  memcpy(pbuf, kv->value, kv->vlength); pbuf += kv->vlength;

  uspace = Z_AVL16_ALIGN(Z_AVL16_NODE_SIZE + (pbuf - node->data));
  head->blk_avail  -= uspace;
  head->kv_count   += 1;
  head->index_next += Z_AVL16_OFF2POS(uspace);

  z_dblock_map_stats_update(&(head->kv_stats), kv);

  return(head->blk_avail);
}

static void __record_get (const uint8_t *block,
                          const uint16_t node_pos,
                          z_dblock_kv_t *kv)
{
  z_avl16_node_t *node = Z_AVL16_NODE(block, node_pos);
  const uint8_t *pbuf = node->data + 1;
  const uint8_t head = node->data[0];
  switch (head & 0xC0) {
    case 1 << 6: /* 01 KK VV VV -> klen=3  vlen=15 */
      kv->klength = 1 + ((head & 0x30) >> 4);
      kv->vlength = (head & 0xf);
      break;
    case 2 << 6: /* 10 KK KV VV -> klen=7  vlen=7 */
      kv->klength = 1 + ((head & 0x38) >> 3);
      kv->vlength = (head & 0x7);
      break;
    case 3 << 6: /* 11 KK KK KK -> klen=63 vlen=0 */
      kv->klength = 1 + (head & 0x3f);
      kv->vlength = 0;
      break;
    case 0 << 6: { /* 00 00 KK VV -> klen=N vlen=N */
      const uint8_t ksize = (head & 0x0c) >> 2;
      const uint8_t vsize = (head & 0x03);
      z_uint32_decode(pbuf, ksize, &(kv->klength)); pbuf += ksize;
      z_uint32_decode(pbuf, vsize, &(kv->vlength)); pbuf += vsize;
      break;
    }
  }
  kv->key   = pbuf;
  kv->value = pbuf + kv->klength;
}

/* ============================================================================
 *  Direct Lookup methods
 */
static int __dblock_avl16_map_lookup (uint8_t *block, z_dblock_kv_t *kv) {
  z_dblock_avl16_map_head_t *head = Z_CAST(z_dblock_avl16_map_head_t, block);
  uint16_t node_pos;

  node_pos = z_avl16_lookup(block, head->avl_head.root,
                            __record_compare, kv, block);
  if (node_pos == 0)
    return(0);

  __record_get(block, node_pos, kv);
  return(1);
}

static void __dblock_avl16_map_first_key (uint8_t *block, z_dblock_kv_t *kv) {
  const z_dblock_avl16_map_head_t *head = Z_CONST_CAST(z_dblock_avl16_map_head_t, block);
  __record_get(block, head->avl_head.edge[0], kv);
}

static void __dblock_avl16_map_last_key (uint8_t *block, z_dblock_kv_t *kv) {
  const z_dblock_avl16_map_head_t *head = Z_CONST_CAST(z_dblock_avl16_map_head_t, block);
  __record_get(block, head->avl_head.edge[1], kv);
}

static void __dblock_log_map_get_iptr (uint8_t *block, uint32_t iptr, z_dblock_kv_t *kv) {
  __record_get(block, iptr, kv);
}

/* ============================================================================
 *  Iterator methods
 */
static int __dblock_avl16_map_seek (z_dblock_map_iter_t *viter,
                                    uint8_t *block,
                                    z_dblock_seek_t seek_pos,
                                    const z_dblock_kv_t *kv)
{
  const z_dblock_avl16_map_head_t *head = Z_CONST_CAST(z_dblock_avl16_map_head_t, block);
  z_avl16_iter_t *iter = Z_CAST(z_avl16_iter_t, viter->data);

  z_avl16_iter_init(iter, block, head->avl_head.root);
  switch (seek_pos) {
    case Z_DBLOCK_SEEK_BEGIN:
      return(!!z_avl16_iter_seek_begin(iter));
    case Z_DBLOCK_SEEK_END:
      return(!!z_avl16_iter_seek_end(iter));
    case Z_DBLOCK_SEEK_LT:
      if (z_avl16_iter_seek_le(iter, __record_compare, kv, block) == NULL)
        return(0);
      return(iter->found ? z_avl16_iter_prev(iter) != NULL : 1);
    case Z_DBLOCK_SEEK_LE:
      return(z_avl16_iter_seek_le(iter, __record_compare, kv, block) != NULL);
    case Z_DBLOCK_SEEK_GT:
      if (z_avl16_iter_seek_ge(iter, __record_compare, kv, block) == NULL)
        return(0);
      return(iter->found ? z_avl16_iter_next(iter) != NULL : 1);
    case Z_DBLOCK_SEEK_GE:
      return(z_avl16_iter_seek_ge(iter, __record_compare, kv, block) != NULL);
    case Z_DBLOCK_SEEK_EQ:
      z_avl16_iter_seek_le(iter, __record_compare, kv, block);
      return(iter->found);
  }
  return(0);
}

static int __dblock_avl16_map_seek_next (z_dblock_map_iter_t *viter) {
  z_avl16_iter_t *iter = Z_CAST(z_avl16_iter_t, viter->data);
  return(z_avl16_iter_next(iter) != NULL);
}

static int __dblock_avl16_map_seek_prev (z_dblock_map_iter_t *viter) {
  z_avl16_iter_t *iter = Z_CAST(z_avl16_iter_t, viter->data);
  return(z_avl16_iter_prev(iter) != NULL);
}

static void __dblock_avl16_map_seek_item (z_dblock_map_iter_t *viter, z_dblock_kv_t *kv) {
  const z_avl16_iter_t *iter = Z_CONST_CAST(z_avl16_iter_t, viter->data);
  __record_get(iter->block, iter->current, kv);
}

static uint32_t __dblock_avl16_map_seek_iptr (z_dblock_map_iter_t *viter) {
  const z_avl16_iter_t *iter = Z_CONST_CAST(z_avl16_iter_t, viter->data);
  return(iter->current);
}

/* ============================================================================
 *  Insert/Append/Prepend methods
 */
static uint32_t __dblock_avl16_map_insert (uint8_t *block, const z_dblock_kv_t *kv) {
  z_dblock_avl16_map_head_t *head = Z_CAST(z_dblock_avl16_map_head_t, block);
  z_avl16_insert(&(head->avl_head), block, head->index_next,
                 __record_compare, kv, block);
  return __record_add(head, block, kv);
}

static uint32_t __dblock_avl16_map_append (uint8_t *block, const z_dblock_kv_t *kv) {
  z_dblock_avl16_map_head_t *head = Z_CAST(z_dblock_avl16_map_head_t, block);
  z_avl16_append(&(head->avl_head), block, head->index_next);
  return __record_add(head, block, kv);
}

static uint32_t __dblock_avl16_map_prepend (uint8_t *block, const z_dblock_kv_t *kv) {
  z_dblock_avl16_map_head_t *head = Z_CAST(z_dblock_avl16_map_head_t, block);
  z_avl16_prepend(&(head->avl_head), block, head->index_next);
  return __record_add(head, block, kv);
}

/* ============================================================================
 *  Stats and utility methods
 */
static int __dblock_avl16_map_has_space (uint8_t *block, const z_dblock_kv_t *kv) {
  const z_dblock_avl16_map_head_t *head = Z_CONST_CAST(z_dblock_avl16_map_head_t, block);
  uint32_t kv_size;
  kv_size = Z_AVL16_NODE_SIZE + 1 +
            z_uint32_size(kv->klength) + z_uint32_size(kv->vlength) +
            kv->klength + kv->vlength;
  return(Z_AVL16_ALIGN(kv_size) <= (head->blk_avail));
}

static uint32_t __dblock_avl16_map_max_overhead (uint8_t *block) {
  return(Z_AVL16_NODE_SIZE + Z_AVL16_ALIGN(1));
}

static void __dblock_avl16_map_stats (uint8_t *block, z_dblock_map_stats_t *stats) {
  const z_dblock_avl16_map_head_t *head = Z_CONST_CAST(z_dblock_avl16_map_head_t, block);
  stats->blk_size  = head->blk_size;
  stats->blk_avail = head->blk_avail;
  stats->kv_count  = head->kv_count;
  stats->is_sorted = 1;
  z_dblock_kv_stats_copy(&(stats->kv_stats), &(head->kv_stats));
}

static void __dblock_avl16_map_dump (uint8_t *block, FILE *stream) {
  const z_dblock_avl16_map_head_t *head = Z_CONST_CAST(z_dblock_avl16_map_head_t, block);
  z_avl16_iter_t iter;
  z_dblock_kv_t kv;
  uint32_t i;

  z_avl16_iter_init(&iter, block, head->avl_head.root);
  z_avl16_iter_seek_begin(&iter);
  for (i = 0; i < head->kv_count; ++i) {
    __record_get(iter.block, iter.current, &kv);
    z_avl16_iter_next(&iter);

    fprintf(stream, " - kv %"PRIu32": ", i);
    z_dblock_kv_dump(&kv, stream);
    fprintf(stream, "\n");
  }
}

static void __dblock_avl16_map_init (uint8_t *block, const z_dblock_map_opts_t *opts) {
  z_dblock_avl16_map_head_t *head = Z_CAST(z_dblock_avl16_map_head_t, block);
  z_avl16_init(&(head->avl_head));
  head->format     = Z_DBLOCK_MAP_TYPE_AVL16;
  head->blk_size   = opts->blk_size;
  head->blk_avail  = opts->blk_size - sizeof(z_dblock_avl16_map_head_t);
  head->kv_count   = 0;
  head->index_next = Z_AVL16_OFF2POS(sizeof(z_dblock_avl16_map_head_t));

  z_dblock_kv_stats_init(&(head->kv_stats));
}

const z_dblock_map_vtable_t z_dblock_avl16_map = {
  .lookup       = __dblock_avl16_map_lookup,
  .first_key    = __dblock_avl16_map_first_key,
  .last_key     = __dblock_avl16_map_last_key,
  .get_iptr     = __dblock_log_map_get_iptr,

  .seek         = __dblock_avl16_map_seek,
  .seek_next    = __dblock_avl16_map_seek_next,
  .seek_prev    = __dblock_avl16_map_seek_prev,
  .seek_item    = __dblock_avl16_map_seek_item,
  .seek_iptr    = __dblock_avl16_map_seek_iptr,

  .insert       = __dblock_avl16_map_insert,
  .append       = __dblock_avl16_map_append,
  .prepend      = __dblock_avl16_map_prepend,

  .has_space    = __dblock_avl16_map_has_space,
  .max_overhead = __dblock_avl16_map_max_overhead,

  .stats        = __dblock_avl16_map_stats,
  .dump         = __dblock_avl16_map_dump,
  .init         = __dblock_avl16_map_init,
};
