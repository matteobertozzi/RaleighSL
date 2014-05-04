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
#include <zcl/memory.h>
#include <zcl/ticket.h>
#include <zcl/dlink.h>
#include <zcl/chmap.h>

Z_TYPEDEF_STRUCT(z_vtable_cache_policy)
Z_TYPEDEF_STRUCT(z_cache_entry)
Z_TYPEDEF_STRUCT(z_cache)

typedef int (*z_cache_evict_t) (void *udata, z_cache_entry_t *entry);

enum z_cache_entry_state {
  Z_CACHE_ENTRY_IS_NEW,
  Z_CACHE_ENTRY_IS_EVICTED,
  Z_CACHE_ENTRY_IS_IN_QUEUE,
};

struct z_vtable_cache_policy {
  void  (*update)  (z_cache_t *self,
                    z_cache_entry_t *entry);
  void  (*remove)  (z_cache_t *self,
                    z_cache_entry_t *entry);
  void  (*reclaim) (z_cache_t *self,
                    uint64_t capacity);

  int   (*create)  (z_cache_t *self,
                    z_memory_t *memory);
  void  (*destroy) (z_cache_t *self,
                    z_memory_t *memory);
  void  (*dump)    (const z_cache_t *self,
                    FILE *stream);

  const char *name;
};

struct z_cache_entry {
  z_chmap_entry_t map_entry;
  z_dlink_node_t link;
};

struct z_cache {
  z_chmap_t *map;
  uint64_t hit;
  uint64_t miss;

  uint64_t capacity;
  uint64_t usage;
  const z_vtable_cache_policy_t *vpolicy;

  union policy {
    void *data;
    struct q {
      z_ticket_t lock;
      z_dlink_node_t head;
    } q;
  } policy;

  z_cache_evict_t entry_evict;
  z_mem_free_t entry_free;
  void *udata;
};

extern const z_vtable_cache_policy_t z_cache_policy_lru;
extern const z_vtable_cache_policy_t z_cache_policy_mru;

#define z_cache_entry_oid(entry)      ((entry)->map_entry.oid)

void              z_cache_entry_init (z_cache_entry_t *entry, uint64_t oid);

int               z_cache_alloc      (z_cache_t *cache,
                                      const z_vtable_cache_policy_t *vpolicy,
                                      uint64_t capacity,
                                      z_cache_evict_t entry_evict,
                                      z_mem_free_t entry_free,
                                      void *user_data);
void              z_cache_free       (z_cache_t *self);
z_cache_entry_t * z_cache_try_insert (z_cache_t *self, z_cache_entry_t *entry);
z_cache_entry_t * z_cache_lookup     (z_cache_t *self, uint64_t oid);
z_cache_entry_t * z_cache_remove     (z_cache_t *self, uint64_t oid);
void              z_cache_release    (z_cache_t *self, z_cache_entry_t *entry);
void              z_cache_reclaim    (z_cache_t *self, z_dlink_node_t *entries);
void              z_cache_dump       (const z_cache_t *self, FILE *stream);

__Z_END_DECLS__

#endif /* !_Z_CACHE_H_ */
