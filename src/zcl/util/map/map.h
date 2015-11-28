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

#ifndef _Z_MAP_H_
#define _Z_MAP_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

typedef int (*z_compare_t) (void *udata, const void *a, const void *b);

uint32_t  z_map_chain_k64_init   (uint8_t *block, uint32_t size,
                                  uint32_t nbuckets, uint32_t isize);
void *    z_map_chain_k64_get    (uint8_t *block, uint32_t hash, uint64_t key);
void *    z_map_chain_k64_put    (uint8_t *block, uint32_t hash, uint64_t key);
void *    z_map_chain_k64_remove (uint8_t *block, uint32_t hash, uint64_t key);

uint32_t  z_map_chain_a64_init   (uint8_t *block, uint32_t size,
                                  uint32_t nbuckets, uint32_t isize);
void *    z_map_chain_a32_get    (uint8_t *block, uint32_t hash,
                                  z_compare_t kcmp, const void *key, void *udata);
void *    z_map_chain_a32_put    (uint8_t *block, uint32_t hash,
                                  z_compare_t kcmp, const void *key, void *udata);
void *    z_map_chain_a32_remove (uint8_t *block, uint32_t hash,
                                  z_compare_t kcmp, const void *key, void *udata);

__Z_END_DECLS__

#endif /* !_Z_MAP_H_ */
