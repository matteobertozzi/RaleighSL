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

#ifndef _Z_DBLOCK_KV_H_
#define _Z_DBLOCK_KV_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

typedef struct z_dblock_kv_stats z_dblock_kv_stats_t;
typedef struct z_dblock_kv z_dblock_kv_t;

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
  uint32_t kv_count;
};

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
    (stats)->kv_count    = 0;               \
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

#define z_dblock_kv_stats_update(stats, kv)                                   \
  do {                                                                        \
    (stats)->kv_count++;                                                      \
    z_min_max(&((stats)->ksize_min), &((stats)->ksize_max), (kv)->klength);   \
    (stats)->ksize_total += (kv)->klength;                                    \
    z_min_max(&((stats)->vsize_min), &((stats)->vsize_max), (kv)->vlength);   \
    (stats)->vsize_total += (kv)->vlength;                                    \
  } while (0)

__Z_END_DECLS__

#endif /* !_Z_DBLOCK_KV_H_ */
