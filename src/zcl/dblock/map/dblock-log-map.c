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

/* TODO PREFIX COMPRESSION */

typedef struct z_dblock_log_map_head z_dblock_log_map_head_t;
typedef struct z_dblock_log_map_iter z_dblock_log_map_iter_t;

struct z_dblock_log_map_head {
  /* read vars */
  uint32_t blk_size;
  uint32_t kv_edge[2];
  /* write vars */
  uint32_t blk_used;
  uint32_t kv_last;
  uint32_t is_sorted : 1;
  z_dblock_kv_stats_t kv_stats;
} __attribute__((packed));

struct z_dblock_log_map_iter {
  const uint8_t *block;
  const uint8_t *prec;
  const uint8_t *pnext;
};

/* ============================================================================
 *  Direct Lookup methods
 */
static int __dblock_log_map_lookup (uint8_t *block, z_dblock_kv_t *kv) {
  const z_dblock_log_map_head_t *head = Z_CONST_CAST(z_dblock_log_map_head_t, block);
  z_dblock_kv_t iter_kv;
  const uint8_t *pbuf;
  uint32_t kv_count;

  kv_count = head->kv_stats.kv_count;
  pbuf = block + sizeof(z_dblock_log_map_head_t);
  while (kv_count--) {
    pbuf = z_dblock_log_map_record_get(block, pbuf, &iter_kv);
    if (z_mem_equals(iter_kv.key, iter_kv.klength, kv->key, kv->klength)) {
      z_dblock_kv_copy(kv, &iter_kv);
      return(1);
    }
  }
  return(0);
}

static void __dblock_log_map_first_key (uint8_t *block, z_dblock_kv_t *kv) {
  const z_dblock_log_map_head_t *head = Z_CONST_CAST(z_dblock_log_map_head_t, block);
  z_dblock_log_map_record_get(block, block + head->kv_edge[0], kv);
}

static void __dblock_log_map_last_key (uint8_t *block, z_dblock_kv_t *kv) {
  const z_dblock_log_map_head_t *head = Z_CONST_CAST(z_dblock_log_map_head_t, block);
  z_dblock_log_map_record_get(block, block + head->kv_edge[1], kv);
}

static void __dblock_log_map_get_iptr (uint8_t *block, uint32_t iptr, z_dblock_kv_t *kv) {
  z_dblock_log_map_record_get(block, block + iptr, kv);
}

/* ============================================================================
 *  Iterator methods
 */
static int __dblock_log_map_seek (z_dblock_iter_t *viter,
                                  uint8_t *block,
                                  z_dblock_seek_t seek_pos,
                                  const z_dblock_kv_t *kv)
{
  const z_dblock_log_map_head_t *head = Z_CONST_CAST(z_dblock_log_map_head_t, block);
  z_dblock_log_map_iter_t *iter = Z_CAST(z_dblock_log_map_iter_t, viter->data);

  iter->block = block;
  if (seek_pos == Z_DBLOCK_SEEK_BEGIN) {
    iter->prec = block + sizeof(z_dblock_log_map_head_t);
  } else if (seek_pos == Z_DBLOCK_SEEK_END) {
    iter->prec = block + head->kv_last;
  } else {
    z_dblock_kv_t iter_kv;
    const uint8_t *plast;
    const uint8_t *pcur;
    uint32_t kv_count;
    int cmp;

    pcur = block + sizeof(z_dblock_log_map_head_t);
    kv_count = head->kv_stats.kv_count;
    while (kv_count--) {
      plast = pcur;
      pcur = z_dblock_log_map_record_get(block, pcur, &iter_kv);
      cmp = z_mem_compare(iter_kv.key, iter_kv.klength, kv->key, kv->klength);
      if (cmp < 0) continue;

      switch (seek_pos) {
        case Z_DBLOCK_SEEK_LE:
          if (cmp == 0) {
            iter->prec = plast;
            iter->pnext = pcur;
            return(1);
          }
        case Z_DBLOCK_SEEK_LT: {
          const uint8_t *pprev = z_dblock_log_map_record_prev(block, plast);
          if (pprev != NULL && plast > pprev) {
            iter->prec = pprev;
            iter->pnext = plast;
            return(1);
          }
          return(0);
        }
        case Z_DBLOCK_SEEK_GE:
          if (cmp == 0) {
            iter->prec = plast;
            iter->pnext = pcur;
            return(1);
          }
        case Z_DBLOCK_SEEK_GT:
          if (pcur > (block + head->kv_last)) {
            return(0);
          }
          iter->prec = pcur;
          iter->pnext = NULL;
          return(1);
        default:
          iter->prec = plast;
          iter->pnext = pcur;
          return(1);
      }
    }

    switch (seek_pos) {
      case Z_DBLOCK_SEEK_LE:
      case Z_DBLOCK_SEEK_LT:
        iter->prec = plast;
        iter->pnext = NULL;
        return(1);
      default:
        break;
    }
  }
  return(0);
}

static int __dblock_log_map_seek_next (z_dblock_iter_t *viter) {
  z_dblock_log_map_iter_t *iter = Z_CAST(z_dblock_log_map_iter_t, viter->data);
  const z_dblock_log_map_head_t *head;
  head = Z_CONST_CAST(z_dblock_log_map_head_t, iter->block);
  if (iter->pnext != NULL) {
    if ((iter->pnext - iter->block) <= head->kv_last) {
      iter->prec = iter->pnext;
      iter->pnext = NULL;
      return(1);
    }
    return(0);
  }
  if ((iter->prec - iter->block) < head->kv_last) {
    z_dblock_kv_t iter_kv;
    iter->prec = z_dblock_log_map_record_get(iter->block, iter->prec, &iter_kv);
    return(1);
  }
  return(0);
}

static int __dblock_log_map_seek_prev (z_dblock_iter_t *viter) {
  z_dblock_log_map_iter_t *iter = Z_CAST(z_dblock_log_map_iter_t, viter->data);
  iter->prec = z_dblock_log_map_record_prev(iter->block, iter->prec);
  return(iter->prec != NULL);
}

static void __dblock_log_map_seek_item (z_dblock_iter_t *viter, z_dblock_kv_t *kv) {
  z_dblock_log_map_iter_t *iter = Z_CAST(z_dblock_log_map_iter_t, viter->data);
  iter->pnext = z_dblock_log_map_record_get(iter->block, iter->prec, kv);
}

static uint32_t __dblock_log_map_seek_iptr (z_dblock_iter_t *viter) {
  z_dblock_log_map_iter_t *iter = Z_CAST(z_dblock_log_map_iter_t, viter->data);
  return((iter->prec - iter->block) & 0xffffffff);
}

/* ============================================================================
 *  Insert/Append/Prepend methods
 */
static uint32_t __dblock_log_map_prepend (uint8_t *block, const z_dblock_kv_t *kv) {
  z_dblock_log_map_head_t *head = Z_CAST(z_dblock_log_map_head_t, block);
  z_dblock_log_map_record_add(block, &(head->blk_used), &(head->kv_last), &(head->kv_stats), kv);
  head->kv_edge[0] = head->kv_last;
  head->is_sorted  = 0;
  return(head->blk_size - head->blk_used);
}

static uint32_t __dblock_log_map_append (uint8_t *block, const z_dblock_kv_t *kv) {
  z_dblock_log_map_head_t *head = Z_CAST(z_dblock_log_map_head_t, block);
  z_dblock_log_map_record_add(block, &(head->blk_used), &(head->kv_last), &(head->kv_stats), kv);
  head->kv_edge[1] = head->kv_last;
  return(head->blk_size - head->blk_used);
}

static uint32_t __dblock_log_map_insert (uint8_t *block, const z_dblock_kv_t *kv) {
  z_dblock_log_map_head_t *head = Z_CAST(z_dblock_log_map_head_t, block);
  if (Z_LIKELY(head->kv_stats.kv_count > 0)) {
    z_dblock_kv_t ikv;

    __dblock_log_map_last_key(block, &ikv);
    if (z_mem_compare(kv->key, kv->klength, ikv.key, ikv.klength) > 0)
      return __dblock_log_map_append(block, kv);

    __dblock_log_map_first_key(block, &ikv);
    if (z_mem_compare(kv->key, kv->klength, ikv.key, ikv.klength) < 0)
      return __dblock_log_map_prepend(block, kv);

    z_dblock_log_map_record_add(block, &(head->blk_used), &(head->kv_last), &(head->kv_stats), kv);
    head->is_sorted = 0;
    return(head->blk_size - head->blk_used);
  }
  return __dblock_log_map_append(block, kv);
}

/* ============================================================================
 *  Stats and utility methods
 */
static int __dblock_log_map_has_space (uint8_t *block, const z_dblock_kv_t *kv) {
  const z_dblock_log_map_head_t *head = Z_CONST_CAST(z_dblock_log_map_head_t, block);
  return(z_dblock_log_map_kv_space(kv) <= (head->blk_size - head->blk_used));
}

static uint32_t __dblock_log_map_max_overhead (uint8_t *block) {
  return(Z_DBLOCK_LOG_MAP_MAX_OVERHEAD);
}

static void __dblock_log_map_stats (uint8_t *block, z_dblock_stats_t *stats) {
  const z_dblock_log_map_head_t *head = Z_CONST_CAST(z_dblock_log_map_head_t, block);
  stats->blk_size  = head->blk_size;
  stats->blk_avail = head->blk_size - head->blk_used;
  stats->is_sorted = head->is_sorted;
  z_dblock_kv_stats_copy(&(stats->kv_stats), &(head->kv_stats));
}

static void __dblock_log_map_init (uint8_t *block, const z_dblock_opts_t *opts) {
  z_dblock_log_map_head_t *head = Z_CAST(z_dblock_log_map_head_t, block);

  head->blk_size   = opts->blk_size;
  head->kv_edge[0] = sizeof(z_dblock_log_map_head_t);
  head->kv_edge[1] = sizeof(z_dblock_log_map_head_t);

  head->blk_used   = sizeof(z_dblock_log_map_head_t);
  head->kv_last    = 0;
  head->is_sorted  = 1;

  z_dblock_kv_stats_init(&(head->kv_stats));
}

const z_dblock_vtable_t z_dblock_log_map = {
  .lookup       = __dblock_log_map_lookup,
  .first_key    = __dblock_log_map_first_key,
  .last_key     = __dblock_log_map_last_key,
  .get_iptr     = __dblock_log_map_get_iptr,

  .seek         = __dblock_log_map_seek,
  .seek_next    = __dblock_log_map_seek_next,
  .seek_prev    = __dblock_log_map_seek_prev,
  .seek_item    = __dblock_log_map_seek_item,
  .seek_iptr    = __dblock_log_map_seek_iptr,

  .insert       = __dblock_log_map_insert,
  .append       = __dblock_log_map_append,
  .prepend      = __dblock_log_map_prepend,
  .remove       = NULL,
  .replace      = NULL,

  .has_space    = __dblock_log_map_has_space,
  .max_overhead = __dblock_log_map_max_overhead,

  .stats        = __dblock_log_map_stats,
  .init         = __dblock_log_map_init,
};
