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

#include <zcl/global.h>
#include <zcl/cache.h>

/* ============================================================================
 *  PUBLIC Cache Entry methods
 */
void z_cache_entry_init (z_cache_entry_t *entry, uint64_t oid) {
  z_chmap_entry_init(&(entry->map_entry), oid);
  entry->map_entry.ustate = Z_CACHE_ENTRY_IS_NEW;
  entry->map_entry.uweight = 1;
  z_dlink_init(&(entry->link));
}

/* ============================================================================
 *  PUBLIC Cache methods
 */
int z_cache_alloc (z_cache_t *self,
                   const z_vtable_cache_policy_t *vpolicy,
                   uint64_t capacity,
                   z_cache_evict_t entry_evict,
                   z_mem_free_t entry_free,
                   void *user_data)
{
  z_memory_t *memory = z_global_memory();

  self->map = z_chmap_create(memory, capacity);
  if (Z_UNLIKELY(self->map == NULL))
    return(-1);

  self->hit = 0;
  self->miss = 0;
  self->capacity = capacity;
  self->usage = 0;

  self->vpolicy = vpolicy;
  if (self->vpolicy->create(self, memory)) {
    z_chmap_destroy(memory, self->map);
    return(-2);
  }

  self->entry_evict = entry_evict;
  self->entry_free = entry_free;
  self->udata = user_data;
  return(0);
}

void z_cache_free (z_cache_t *self) {
  z_memory_t *memory = z_global_memory();
  self->vpolicy->destroy(self, memory);
  z_chmap_destroy(memory, self->map);
}

z_cache_entry_t *z_cache_try_insert (z_cache_t *self, z_cache_entry_t *entry) {
  z_chmap_entry_t *old;

  old = z_chmap_try_insert(self->map, &(entry->map_entry));
  if (old != NULL) {
    z_cache_entry_t *old_entry = z_container_of(old, z_cache_entry_t, map_entry);
    self->vpolicy->update(self, old_entry);
    return(old_entry);
  }

  self->vpolicy->update(self, entry);
  return(NULL);
}

z_cache_entry_t *z_cache_lookup (z_cache_t *self, uint64_t oid) {
  z_chmap_entry_t *map_entry;

  map_entry = z_chmap_lookup(self->map, oid);
  if (map_entry != NULL) {
    z_cache_entry_t *entry = z_container_of(map_entry, z_cache_entry_t, map_entry);
    self->vpolicy->update(self, entry);
    z_atomic_inc(&(self->hit));
    Z_LOG_TRACE("Cache Loookup %"PRIu64" refs %u", entry->map_entry.oid, entry->map_entry.refs);
    return(entry);
  }

  z_atomic_inc(&(self->miss));
  return(NULL);
}

z_cache_entry_t *z_cache_remove (z_cache_t *self, uint64_t oid) {
  z_chmap_entry_t *map_entry;

  Z_LOG_TRACE("Cache Remove %"PRIu64, oid);
  map_entry = z_chmap_remove(self->map, oid);
  if (map_entry != NULL) {
    z_cache_entry_t *entry = z_container_of(map_entry, z_cache_entry_t, map_entry);
    self->vpolicy->remove(self, entry);
    return(entry);
  }
  return(NULL);
}

void z_cache_release (z_cache_t *self, z_cache_entry_t *entry) {
  Z_ASSERT(entry->map_entry.refs > 0, "Entry already unreferenced");
  if (z_atomic_dec(&(entry->map_entry.refs)) == 0) {
    Z_LOG_TRACE("Cache Release %"PRIu64, entry->map_entry.oid);

    if (self->entry_free != NULL) {
      self->entry_free(self->udata, entry);
    }
  }
}

void z_cache_reclaim (z_cache_t *self, z_dlink_node_t *entries) {
  z_cache_entry_t *entry;
  z_dlink_del_for_each_entry(entries, entry, z_cache_entry_t, link, {
    z_chmap_remove_entry(self->map, &(entry->map_entry));
    z_cache_release(self, entry);
  });
}

void z_cache_dump (const z_cache_t *self, FILE *stream) {
  self->vpolicy->dump(self, stream);
  fprintf(stream,
          " - hit %"PRIu64" miss %"PRIu64" (%.2f hit)\n"
          " - usage %"PRIu64" capacity %"PRIu64"\n\n",
          self->hit, self->miss, (double)self->hit / (self->hit + self->miss),
          self->usage, self->capacity);
}
