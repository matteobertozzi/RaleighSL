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
#include <zcl/bits.h>

/* TODO PREFIX COMPRESSION */

typedef struct z_dblock_log_map_head z_dblock_log_map_head_t;
typedef struct z_dblock_log_map_iter z_dblock_log_map_iter_t;

struct z_dblock_log_map_head {
  uint8_t format;
  uint8_t _pad[3];

  /* read vars */
  uint32_t blk_size;
  uint32_t kv_count;
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
 *  PRIVATE record helpers
 */
static void __record_add (uint8_t *block, const z_dblock_kv_t *kv) {
  z_dblock_log_map_head_t *head = Z_CAST(z_dblock_log_map_head_t, block);
  uint32_t kprefix;
  uint8_t *rhead;
  uint8_t *pbuf;

  /* Z_ASSERT(kv->klength > 0) */

  rhead  = block + head->blk_used;
  pbuf   = rhead + 1;
  *rhead = 0;

  kprefix = 0; // TODO

  if (Z_LIKELY(head->kv_count > 0)) {
    uint32_t pprev = head->blk_used - head->kv_last;
    const uint8_t size = z_uint32_size(pprev);
    *rhead |= size << 6;
    z_uint32_encode(pbuf, size, pprev);
    pbuf += size;
  }

  if (kprefix > 0) {
    const uint32_t size = z_uint32_size(kprefix);
    *rhead |= size << 4;
    z_uint32_encode(pbuf, size, kprefix);
    pbuf += size;
  }

  if (((kv->klength - 1) + kv->vlength) <= 66) {
    int size = 0;
    if (kv->vlength > 0) {
      size = (32 - z_clz32(kv->vlength));
      size = z_align_up(size, 2);
    }
    *rhead |= (size >> 1);
    *pbuf++ = ((kv->klength - 1) << size) | kv->vlength;
  } else {
    const uint8_t ksize = z_uint32_size(kv->klength);
    const uint8_t vsize = z_uint32_size(kv->vlength);

    *rhead |= (ksize << 2) | vsize;
    z_uint32_encode(pbuf, ksize, kv->klength); pbuf += ksize;
    z_uint32_encode(pbuf, vsize, kv->vlength); pbuf += vsize;
  }

  memcpy(pbuf, kv->key,   kv->klength); pbuf += kv->klength;
  memcpy(pbuf, kv->value, kv->vlength); pbuf += kv->vlength;

  head->kv_count += 1;
  head->blk_used  =  (pbuf - block) & 0xffffffff;
  head->kv_last   = (rhead - block) & 0xffffffff;

  z_dblock_map_stats_update(&(head->kv_stats), kv);
}

static const uint8_t *__record_get (const uint8_t *block,
                                    const uint8_t *recbuf,
                                    z_dblock_kv_t *kv)
{
  uint8_t head;

  head = *recbuf;

  // skip head + pprev
  recbuf += 1 + (((head & 0xC0) >> 6) & 0x3);

  // TODO: kprefix

  if ((head & 0x0C) == 0) {
    const int size = (head & 0x3) << 1;
    kv->klength = 1 + ((*recbuf & 0xff) >> size);
    kv->vlength = (*recbuf & 0xff) & ((1 << size) - 1);
    recbuf++;
  } else {
    const uint8_t ksize = (head & 0x0C) >> 2;
    const uint8_t vsize = (head & 0x03);
    z_uint32_decode(recbuf, ksize, &(kv->klength)); recbuf += ksize;
    z_uint32_decode(recbuf, vsize, &(kv->vlength)); recbuf += vsize;
  }

  kv->key   = recbuf; recbuf += kv->klength;
  kv->value = recbuf; recbuf += kv->vlength;
  return(recbuf);
}

static const uint8_t *__record_prev (const uint8_t *block,
                                     const uint8_t *recbuf)
{
  const int size = ((*recbuf & 0xC0) >> 6) & 0x3;
  if (size > 0) {
    uint32_t pprev;
    z_uint32_decode(recbuf + 1, size, &pprev);
    return(recbuf - pprev);
  }
  return(NULL);
}

/* ============================================================================
 *  Direct Lookup methods
 */
static int __dblock_log_map_lookup (uint8_t *block, z_dblock_kv_t *kv) {
  const z_dblock_log_map_head_t *head = Z_CONST_CAST(z_dblock_log_map_head_t, block);
  z_dblock_kv_t iter_kv;
  const uint8_t *pbuf;
  uint32_t kv_count;

  kv_count = head->kv_count;
  pbuf = block + sizeof(z_dblock_log_map_head_t);
  while (kv_count--) {
    pbuf = __record_get(block, pbuf, &iter_kv);
    if (z_mem_equals(iter_kv.key, iter_kv.klength, kv->key, kv->klength)) {
      z_dblock_kv_copy(kv, &iter_kv);
      return(1);
    }
  }
  return(0);
}

static void __dblock_log_map_first_key (uint8_t *block, z_dblock_kv_t *kv) {
  const z_dblock_log_map_head_t *head = Z_CONST_CAST(z_dblock_log_map_head_t, block);
  __record_get(block, block + head->kv_edge[0], kv);
}

static void __dblock_log_map_last_key (uint8_t *block, z_dblock_kv_t *kv) {
  const z_dblock_log_map_head_t *head = Z_CONST_CAST(z_dblock_log_map_head_t, block);
  __record_get(block, block + head->kv_edge[1], kv);
}

static void __dblock_log_map_get_iptr (uint8_t *block, uint32_t iptr, z_dblock_kv_t *kv) {
  __record_get(block, block + iptr, kv);
}

/* ============================================================================
 *  Iterator methods
 */
static int __dblock_log_map_seek (z_dblock_map_iter_t *viter,
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
    kv_count = head->kv_count;
    while (kv_count--) {
      plast = pcur;
      pcur = __record_get(block, pcur, &iter_kv);
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
          const uint8_t *pprev = __record_prev(block, plast);
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

static int __dblock_log_map_seek_next (z_dblock_map_iter_t *viter) {
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
    iter->prec = __record_get(iter->block, iter->prec, &iter_kv);
    return(1);
  }
  return(0);
}

static int __dblock_log_map_seek_prev (z_dblock_map_iter_t *viter) {
  z_dblock_log_map_iter_t *iter = Z_CAST(z_dblock_log_map_iter_t, viter->data);
  iter->prec = __record_prev(iter->block, iter->prec);
  return(iter->prec != NULL);
}

static void __dblock_log_map_seek_item (z_dblock_map_iter_t *viter, z_dblock_kv_t *kv) {
  z_dblock_log_map_iter_t *iter = Z_CAST(z_dblock_log_map_iter_t, viter->data);
  iter->pnext = __record_get(iter->block, iter->prec, kv);
}

static uint32_t __dblock_log_map_seek_iptr (z_dblock_map_iter_t *viter) {
  z_dblock_log_map_iter_t *iter = Z_CAST(z_dblock_log_map_iter_t, viter->data);
  return((iter->prec - iter->block) & 0xffffffff);
}

/* ============================================================================
 *  Insert/Append/Prepend methods
 */
static uint32_t __dblock_log_map_prepend (uint8_t *block, const z_dblock_kv_t *kv) {
  z_dblock_log_map_head_t *head = Z_CAST(z_dblock_log_map_head_t, block);
  __record_add(block, kv);
  head->kv_edge[0] = head->kv_last;
  head->is_sorted  = 0;
  return(head->blk_size - head->blk_used);
}

static uint32_t __dblock_log_map_append (uint8_t *block, const z_dblock_kv_t *kv) {
  z_dblock_log_map_head_t *head = Z_CAST(z_dblock_log_map_head_t, block);
  __record_add(block, kv);
  head->kv_edge[1] = head->kv_last;
  return(head->blk_size - head->blk_used);
}

static uint32_t __dblock_log_map_insert (uint8_t *block, const z_dblock_kv_t *kv) {
  z_dblock_log_map_head_t *head = Z_CAST(z_dblock_log_map_head_t, block);
  if (head->kv_count > 0) {
    z_dblock_kv_t ikv;

    __dblock_log_map_last_key(block, &ikv);
    if (z_mem_compare(kv->key, kv->klength, ikv.key, ikv.klength) > 0)
      return __dblock_log_map_append(block, kv);

    __dblock_log_map_first_key(block, &ikv);
    if (z_mem_compare(kv->key, kv->klength, ikv.key, ikv.klength) < 0)
      return __dblock_log_map_prepend(block, kv);

    __record_add(block, kv);
    head->is_sorted = 0;
    return(head->blk_size - head->blk_used);
  } else {
    return __dblock_log_map_append(block, kv);
  }
}

/* ============================================================================
 *  Stats and utility methods
 */
static int __dblock_log_map_has_space (uint8_t *block, const z_dblock_kv_t *kv) {
  const z_dblock_log_map_head_t *head = Z_CONST_CAST(z_dblock_log_map_head_t, block);
  uint32_t kv_size;
  kv_size = 4 + z_uint32_size(kv->klength) + z_uint32_size(kv->vlength) +
            kv->klength + kv->vlength;
  return(kv_size <= (head->blk_size - head->blk_used));
}

static uint32_t __dblock_log_map_max_overhead (uint8_t *block) {
  return(1 + 3 + 3 + 3);
}

static void __dblock_log_map_stats (uint8_t *block, z_dblock_map_stats_t *stats) {
  const z_dblock_log_map_head_t *head = Z_CONST_CAST(z_dblock_log_map_head_t, block);
  stats->blk_size  = head->blk_size;
  stats->blk_avail = head->blk_size - head->blk_used;
  stats->kv_count  = head->kv_count;
  z_dblock_kv_stats_copy(&(stats->kv_stats), &(head->kv_stats));
}

static void __dblock_log_map_dump (uint8_t *block, FILE *stream) {
  const z_dblock_log_map_head_t *head = Z_CONST_CAST(z_dblock_log_map_head_t, block);
  const uint8_t *pbuf;
  z_dblock_kv_t kv;
  uint32_t i;

  pbuf = block + sizeof(z_dblock_log_map_head_t);
  for (i = 0; i < head->kv_count; ++i) {
    pbuf = __record_get(block, pbuf, &kv);

    fprintf(stream, " - kv %"PRIu32": ", i);
    z_dblock_kv_dump(&kv, stream);
    fprintf(stream, "\n");
  }
}

static void __dblock_log_map_init (uint8_t *block, const z_dblock_map_opts_t *opts) {
  z_dblock_log_map_head_t *head = Z_CAST(z_dblock_log_map_head_t, block);
  head->format     = Z_DBLOCK_MAP_TYPE_LOG;
  head->blk_size   = opts->blk_size;
  head->kv_count   = 0;
  head->kv_edge[0] = sizeof(z_dblock_log_map_head_t);
  head->kv_edge[1] = sizeof(z_dblock_log_map_head_t);

  head->blk_used   = sizeof(z_dblock_log_map_head_t);
  head->kv_last    = sizeof(z_dblock_log_map_head_t);
  head->is_sorted  = 1;

  z_dblock_kv_stats_init(&(head->kv_stats));
}

const z_dblock_map_vtable_t z_dblock_log_map = {
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

  .has_space    = __dblock_log_map_has_space,
  .max_overhead = __dblock_log_map_max_overhead,

  .stats        = __dblock_log_map_stats,
  .dump         = __dblock_log_map_dump,
  .init         = __dblock_log_map_init,
};
