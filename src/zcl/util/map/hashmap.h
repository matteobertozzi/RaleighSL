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

#ifndef _Z_HASH_MAP_H_
#define _Z_HASH_MAP_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/array.h>

typedef struct z_hash_map z_hash_map_t;

struct z_hash_map {
  z_array_t   entries;
  uint32_t *  buckets;
  uint32_t    nbuckets;
  uint32_t    mask;
  uint32_t    items;
  uint32_t    free_list;
};

int   z_hash_map_open     (z_hash_map_t *self,
                           uint16_t isize,
                           uint32_t capacity,
                           uint32_t bucket_pagesz,
                           uint32_t entries_pagesz);
void  z_hash_map_close    (z_hash_map_t *self);
void *z_hash_map_get      (const z_hash_map_t *self,
                           uint32_t hash,
                           z_compare_t key_cmp,
                           const void *key,
                           void *udata);
void *z_hash_map_put      (z_hash_map_t *self,
                           uint32_t hash,
                           z_compare_t key_cmp,
                           const void *key,
                           void *udata);
void *z_hash_map_remove   (z_hash_map_t *self,
                           uint32_t hash,
                           z_compare_t key_cmp,
                           const void *key,
                           void *udata);

__Z_END_DECLS__

#endif /* !_Z_HASH_MAP_H_ */
