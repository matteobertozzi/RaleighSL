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

#ifndef _Z_CHAIN_MAP_H_
#define _Z_CHAIN_MAP_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

#define z_chain_map_init    z_chain32_map_init
#define z_chain_map_get     z_chain32_map_get
#define z_chain_map_put     z_chain32_map_put
#define z_chain_map_remove  z_chain32_map_remove

uint32_t  z_chain16_map_init    (uint8_t *block,
                                 uint32_t size,
                                 uint16_t isize,
                                 uint16_t bucket_div);
void *    z_chain16_map_get     (uint8_t *block,
                                 uint32_t hash,
                                 z_compare_t key_cmp,
                                 const void *key,
                                 void *udata);
void *    z_chain16_map_put     (uint8_t *block,
                                 uint32_t hash,
                                 z_compare_t key_cmp,
                                 const void *key,
                                 void *udata);
void *    z_chain16_map_remove  (uint8_t *block,
                                 uint32_t hash,
                                 z_compare_t key_cmp,
                                 const void *key,
                                 void *udata);

uint32_t  z_chain32_map_init    (uint8_t *block,
                                 uint32_t size,
                                 uint16_t isize,
                                 uint16_t bucket_div);
void *    z_chain32_map_get     (uint8_t *block,
                                 uint32_t hash,
                                 z_compare_t key_cmp,
                                 const void *key,
                                 void *udata);
void *    z_chain32_map_put     (uint8_t *block,
                                 uint32_t hash,
                                 z_compare_t key_cmp,
                                 const void *key,
                                 void *udata);
void *    z_chain32_map_remove  (uint8_t *block,
                                 uint32_t hash,
                                 z_compare_t key_cmp,
                                 const void *key,
                                 void *udata);

__Z_END_DECLS__

#endif /* !_Z_CHAIN_MAP_H_ */
