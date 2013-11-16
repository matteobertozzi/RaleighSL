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

#ifndef _Z_CACHE_H_
#define _Z_CACHE_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/dlink.h>
#include <stdio.h>

#define __Z_CACHE_ENTRY__               z_cache_entry_t __cache_entry__;

#define Z_CACHE_ENTRY(x)                Z_CAST(z_cache_entry_t, x)
#define Z_CACHE(x)                      Z_CAST(z_cache_t, x)

Z_TYPEDEF_ENUM(z_cache_type)
Z_TYPEDEF_STRUCT(z_cache_entry)
Z_TYPEDEF_STRUCT(z_cache_policy)
Z_TYPEDEF_STRUCT(z_cache)

typedef int (*z_cache_evict_t) (void *udata,
                                unsigned int size,
                                z_cache_entry_t *entry);

struct z_cache_entry {
  z_cache_entry_t *hash;
  z_dlink_node_t cache;

  uint64_t oid;
  uint32_t refs;
  uint32_t state;
};

enum z_cache_type {
  Z_CACHE_LRU,
  Z_CACHE_2Q,
};

struct z_cache_policy {
  void (*insert)  (z_cache_t *cache,
                   z_cache_entry_t *entry);
  void (*update)  (z_cache_t *cache,
                   z_cache_entry_t *entry);
  void (*remove)  (z_cache_t *cache,
                   z_cache_entry_t *entry);
  void (*reclaim) (z_cache_t *cache);
  void (*init)    (z_cache_t *cache);
  void (*destroy) (z_cache_t *cache);
  void (*dump)    (FILE *stream, z_cache_t *cache);

  z_cache_type_t type;
};

#define z_cache_entry_oid(entry)     (Z_CACHE_ENTRY(entry)->oid)

void              z_cache_entry_init (z_cache_entry_t *entry,
                                      uint64_t oid);

z_cache_t *       z_cache_alloc      (z_cache_type_t type,
                                      unsigned int capacity,
                                      z_mem_free_t entry_free,
                                      z_cache_evict_t entry_evict,
                                      void *user_data);
void              z_cache_free       (z_cache_t *cache);
void              z_cache_release    (z_cache_t *cache, z_cache_entry_t *entry);
z_cache_entry_t * z_cache_try_insert (z_cache_t *cache, z_cache_entry_t *entry);
z_cache_entry_t * z_cache_lookup     (z_cache_t *cache, uint64_t oid);
z_cache_entry_t * z_cache_remove     (z_cache_t *cache, uint64_t oid);
void              z_cache_dump       (FILE *stream, z_cache_t *cache);
__Z_END_DECLS__

#endif /* _Z_CACHE_H_ */
