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

#ifndef _Z_DBLOCK_H_
#define _Z_DBLOCK_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/dblock-kv.h>

typedef struct z_dblock_vtable z_dblock_vtable_t;
typedef struct z_dblock_stats z_dblock_stats_t;
typedef struct z_dblock_opts z_dblock_opts_t;
typedef struct z_dblock_iter z_dblock_iter_t;

typedef enum z_dblock_overlap z_dblock_overlap_t;
typedef enum z_dblock_seek z_dblock_seek_t;
typedef enum z_dblock_type z_dblock_type_t;

/* ============================================================================
 */
enum z_dblock_seek {
  Z_DBLOCK_SEEK_BEGIN,
  Z_DBLOCK_SEEK_END,
  Z_DBLOCK_SEEK_LT,
  Z_DBLOCK_SEEK_LE,
  Z_DBLOCK_SEEK_GT,
  Z_DBLOCK_SEEK_GE,
  Z_DBLOCK_SEEK_EQ,
};

enum z_dblock_overlap {
  Z_DBLOCK_OVERLAP_NO        = 1 << 0,
  Z_DBLOCK_OVERLAP_YES       = 1 << 1,
  Z_DBLOCK_OVERLAP_YES_LEFT  = 1 << 3 | Z_DBLOCK_OVERLAP_YES,
  Z_DBLOCK_OVERLAP_YES_RIGHT = 1 << 4 | Z_DBLOCK_OVERLAP_YES,
  Z_DBLOCK_OVERLAP_NO_LEFT   = 1 << 5 | Z_DBLOCK_OVERLAP_NO,
  Z_DBLOCK_OVERLAP_NO_RIGHT  = 1 << 6 | Z_DBLOCK_OVERLAP_NO,
};

/* ============================================================================
 */
struct z_dblock_stats {
  uint32_t blk_size;
  uint32_t blk_avail;
  uint32_t is_sorted : 1;
  z_dblock_kv_stats_t kv_stats;
};

struct z_dblock_opts {
  uint32_t blk_size;
  uint32_t ksize_min;
  uint32_t ksize_max;
  uint32_t vsize_min;
  uint32_t vsize_max;
};

struct z_dblock_iter {
  uint8_t data[128];
};

struct z_dblock_vtable {
  int       (*lookup)       (uint8_t *block, z_dblock_kv_t *kv);
  void      (*first_key)    (uint8_t *block, z_dblock_kv_t *kv);
  void      (*last_key)     (uint8_t *block, z_dblock_kv_t *kv);
  void      (*get_iptr)     (uint8_t *block, uint32_t iptr, z_dblock_kv_t *kv);

  int       (*seek)         (z_dblock_iter_t *viter,
                             uint8_t *block,
                             z_dblock_seek_t seek_pos,
                             const z_dblock_kv_t *key);
  int       (*seek_next)    (z_dblock_iter_t *viter);
  int       (*seek_prev)    (z_dblock_iter_t *viter);
  void      (*seek_item)    (z_dblock_iter_t *viter, z_dblock_kv_t *kv);
  uint32_t  (*seek_iptr)    (z_dblock_iter_t *viter);

  uint32_t  (*insert)       (uint8_t *block, const z_dblock_kv_t *kv);
  uint32_t  (*append)       (uint8_t *block, const z_dblock_kv_t *kv);
  uint32_t  (*prepend)      (uint8_t *block, const z_dblock_kv_t *kv);
  void      (*remove)       (uint8_t *block, const z_dblock_kv_t *kv);
  int       (*replace)      (uint8_t *block,
                             const z_dblock_kv_t *key,
                             const z_dblock_kv_t *kv);

  int       (*has_space)    (uint8_t *block, const z_dblock_kv_t *kv);
  uint32_t  (*max_overhead) (uint8_t *block);

  void      (*stats)        (uint8_t *block, z_dblock_stats_t *stats);
  void      (*init)         (uint8_t *block, const z_dblock_opts_t *opts);
};

/* ============================================================================
 */
enum z_dblock_type {
  Z_DBLOCK_MAP_TYPE_AVL16 = 1,
  Z_DBLOCK_MAP_TYPE_LOG   = 2,
};

extern const z_dblock_vtable_t z_dblock_log_map;
extern const z_dblock_vtable_t z_dblock_hash_map;
extern const z_dblock_vtable_t z_dblock_chain_map;
extern const z_dblock_vtable_t z_dblock_avl16_map;
extern const z_dblock_vtable_t z_dblock_avl16e_map;

const z_dblock_vtable_t *z_dblock_vtable (int type);

/* ============================================================================
 */
z_dblock_overlap_t z_dblock_overlap     (const z_dblock_vtable_t *a_vtable,
                                         uint8_t *a_block,
                                         const z_dblock_vtable_t *b_vtable,
                                         uint8_t *b_block);
z_dblock_overlap_t z_dblock_overlap_blk (const z_dblock_kv_t *a_skey,
                                         const z_dblock_kv_t *a_ekey,
                                         const z_dblock_vtable_t *vtable,
                                         uint8_t *block);

/* ============================================================================
 */
void z_dblock_build_index32 (const z_dblock_vtable_t *vtable,
                             uint8_t *block,
                             uint32_t *idx_block);
void z_dblock_dump          (const z_dblock_vtable_t *vtable,
                             uint8_t *block,
                             FILE *stream);

__Z_END_DECLS__

#endif /* !_Z_DBLOCK_H_ */
