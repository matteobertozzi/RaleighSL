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

#ifndef _Z_DBLOCK_MAP_H_
#define _Z_DBLOCK_MAP_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/dblock.h>

typedef struct z_dblock_map_vtable z_dblock_map_vtable_t;
typedef struct z_dblock_map_stats z_dblock_map_stats_t;
typedef struct z_dblock_map_opts z_dblock_map_opts_t;
typedef struct z_dblock_map_iter z_dblock_map_iter_t;
typedef struct z_dblock_kv_stats z_dblock_kv_stats_t;
typedef struct z_dblock_kv z_dblock_kv_t;

typedef enum z_dblock_map_type z_dblock_map_type_t;

struct z_dblock_kv {
  const void *key;
  const void *value;
  uint32_t klength;
  uint32_t vlength;
};

struct z_dblock_kv_stats {
  uint32_t ksize_min;
  uint32_t ksize_max;
  uint32_t ksize_total;
  uint32_t vsize_min;
  uint32_t vsize_max;
  uint32_t vsize_total;
};

struct z_dblock_map_stats {
  uint32_t blk_size;
  uint32_t blk_avail;
  uint32_t kv_count;
  uint32_t is_sorted : 1;
  z_dblock_kv_stats_t kv_stats;
};

struct z_dblock_map_opts {
  uint32_t blk_size;
  uint32_t ksize_min;
  uint32_t ksize_max;
  uint32_t vsize_min;
  uint32_t vsize_max;
};

struct z_dblock_map_iter {
  uint8_t data[128];
};

struct z_dblock_map_vtable {
  int       (*lookup)       (uint8_t *block, z_dblock_kv_t *kv);
  void      (*first_key)    (uint8_t *block, z_dblock_kv_t *kv);
  void      (*last_key)     (uint8_t *block, z_dblock_kv_t *kv);
  void      (*get_iptr)     (uint8_t *block, uint32_t iptr, z_dblock_kv_t *kv);

  int       (*seek)         (z_dblock_map_iter_t *viter,
                             uint8_t *block,
                             z_dblock_seek_t seek_pos,
                             const z_dblock_kv_t *key);
  int       (*seek_next)    (z_dblock_map_iter_t *viter);
  int       (*seek_prev)    (z_dblock_map_iter_t *viter);
  void      (*seek_item)    (z_dblock_map_iter_t *viter, z_dblock_kv_t *kv);
  uint32_t  (*seek_iptr)    (z_dblock_map_iter_t *viter);

  uint32_t  (*insert)       (uint8_t *block, const z_dblock_kv_t *kv);
  uint32_t  (*append)       (uint8_t *block, const z_dblock_kv_t *kv);
  uint32_t  (*prepend)      (uint8_t *block, const z_dblock_kv_t *kv);

  int       (*has_space)    (uint8_t *block, const z_dblock_kv_t *kv);
  uint32_t  (*max_overhead) (uint8_t *block);

  void      (*stats)        (uint8_t *block, z_dblock_map_stats_t *stats);
  void      (*dump)         (uint8_t *block, FILE *stream);
  void      (*init)         (uint8_t *block, const z_dblock_map_opts_t *opts);
};

enum z_dblock_map_type {
  Z_DBLOCK_MAP_TYPE_AVL16 = 1,
  Z_DBLOCK_MAP_TYPE_LOG   = 2,
};

extern const z_dblock_map_vtable_t z_dblock_log_map;
extern const z_dblock_map_vtable_t z_dblock_hash_map;
extern const z_dblock_map_vtable_t z_dblock_chain_map;
extern const z_dblock_map_vtable_t z_dblock_avl16_map;

#define z_dblock_kv_init(kv, key_, value_, klength_, vlength_)      \
  do {                                                              \
    (kv)->key     = key_;                                           \
    (kv)->value   = value_;                                         \
    (kv)->klength = klength_;                                       \
    (kv)->vlength = vlength_;                                       \
  } while (0)

#define z_dblock_kv_copy(kv, other_kv)                              \
  do {                                                              \
    memcpy(kv, other_kv, sizeof(z_dblock_kv_t));                    \
  } while (0)

#define z_dblock_kv_stats_init(stats)       \
  do {                                      \
    (stats)->ksize_min   = 0xffffffff;      \
    (stats)->ksize_max   = 0;               \
    (stats)->ksize_total = 0;               \
    (stats)->vsize_min   = 0xffffffff;      \
    (stats)->vsize_max   = 0;               \
    (stats)->vsize_total = 0;               \
  } while (0)

#define z_dblock_kv_stats_copy(stats, other_stats)                  \
  do {                                                              \
    memcpy(stats, other_stats, sizeof(z_dblock_kv_stats_t));        \
  } while (0)

#define z_dblock_map_stats_update(stats, kv)                                  \
  do {                                                                        \
    z_min_max(&((stats)->ksize_min), &((stats)->ksize_max), (kv)->klength);   \
    (stats)->ksize_total += (kv)->klength;                                    \
    z_min_max(&((stats)->vsize_min), &((stats)->vsize_max), (kv)->vlength);   \
    (stats)->vsize_total += (kv)->vlength;                                    \
  } while (0)

const z_dblock_map_vtable_t *z_dblock_map_block_vtable (const uint8_t *block);

z_dblock_overlap_t z_dblock_map_overlap (const z_dblock_map_vtable_t *a_vtable,
                                         uint8_t *a_block,
                                         const z_dblock_map_vtable_t *b_vtable,
                                         uint8_t *b_block);

void  z_dblock_map_build_index32 (const z_dblock_map_vtable_t *vtable,
                                 uint8_t *block,
                                 uint32_t *idx_block);

void z_dblock_kv_dump (const z_dblock_kv_t *self, FILE *stream);

__Z_END_DECLS__

#endif /* !_Z_DBLOCK_MAP_H_ */
