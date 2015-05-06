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

#ifndef _DBLOCK_MAP_PRIVATE_H_
#define _DBLOCK_MAP_PRIVATE_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/dblock.h>

#define Z_DBLOCK_LOG_MAP_MIN_OVERHEAD       (1 + 3)
#define Z_DBLOCK_LOG_MAP_MAX_OVERHEAD       (1 + 3 + 3 + 3)

void           z_dblock_log_map_record_add  (uint8_t *block,
                                             uint32_t *pnext,
                                             uint32_t *kv_last,
                                             z_dblock_kv_stats_t *stats,
                                             const z_dblock_kv_t *kv);
const uint8_t *z_dblock_log_map_record_get  (const uint8_t *block,
                                             const uint8_t *recbuf,
                                             z_dblock_kv_t *kv);
const uint8_t *z_dblock_log_map_record_prev (const uint8_t *block,
                                             const uint8_t *recbuf);

uint32_t       z_dblock_log_map_kv_space    (const z_dblock_kv_t *kv);

__Z_END_DECLS__

#endif /* !_DBLOCK_MAP_PRIVATE_H_ */
