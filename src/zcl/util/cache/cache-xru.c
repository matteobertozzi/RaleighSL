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

#include <zcl/cache.h>

static uint64_t __xru_reclaim_delta (z_cache_t *self,
                                     z_dlink_node_t *reclaimq,
                                     uint64_t delta)
{
  uint64_t reclaimed = 0;
  while (reclaimed < delta) {
    z_cache_entry_t *entry;
    entry = z_dlink_back_entry(&(self->policy.q.head), z_cache_entry_t, link);
    if (self->entry_evict != NULL && !self->entry_evict(self->udata, entry))
      break;

    z_dlink_move(reclaimq, &(entry->link));
    entry->map_entry.ustate = Z_CACHE_ENTRY_IS_EVICTED;
    reclaimed += entry->map_entry.uweight;
  }
  return(reclaimed);
}

static void __xru_update (z_cache_t *self, z_cache_entry_t *entry, const int lru) {
  z_dlink_node_t reclaimq;

  z_dlink_init(&reclaimq);
  z_ticket_acquire(&(self->policy.q.lock));
  if (entry->map_entry.ustate != Z_CACHE_ENTRY_IS_EVICTED) {
    if (entry->map_entry.ustate == Z_CACHE_ENTRY_IS_NEW) {
      self->usage += entry->map_entry.uweight;
    }

    /* Evict if no space */
    if (self->usage > self->capacity) {
      self->usage -= __xru_reclaim_delta(self, &reclaimq, self->usage - self->capacity);
    }

    /* Move to the head/tail */
    if (lru) {
      z_dlink_move(&(self->policy.q.head), &(entry->link));
    } else {
      z_dlink_move_tail(&(self->policy.q.head), &(entry->link));
    }
    entry->map_entry.ustate = Z_CACHE_ENTRY_IS_IN_QUEUE;
  }
  z_ticket_release(&(self->policy.q.lock));

  /* release reclaimed entries */
  z_cache_reclaim(self, &reclaimq);
}

static void __lru_update (z_cache_t *self, z_cache_entry_t *entry) {
  __xru_update(self, entry, 1);
}

static void __mru_update (z_cache_t *self, z_cache_entry_t *entry) {
  __xru_update(self, entry, 0);
}

static void __xru_remove (z_cache_t *self, z_cache_entry_t *entry) {
  z_ticket_acquire(&(self->policy.q.lock));
  if (entry->map_entry.ustate != Z_CACHE_ENTRY_IS_EVICTED) {
    z_dlink_del(&(entry->link));
    entry->map_entry.ustate = Z_CACHE_ENTRY_IS_EVICTED;
    self->usage -= entry->map_entry.uweight;
  }
  z_ticket_release(&(self->policy.q.lock));

  /* release reclaimed entry */
  z_cache_release(self, entry);
}

static void __xru_reclaim (z_cache_t *self, uint64_t capacity) {
  z_dlink_node_t reclaimq;

  z_dlink_init(&reclaimq);
  z_ticket_acquire(&(self->policy.q.lock));
  if (Z_LIKELY(self->usage > capacity)) {
    self->usage -= __xru_reclaim_delta(self, &reclaimq, self->usage - capacity);
  }
  z_ticket_release(&(self->policy.q.lock));

  /* release reclaimed entries */
  z_cache_reclaim(self, &reclaimq);
}

static int __xru_create (z_cache_t *self, z_memory_t *memory) {
  z_ticket_init(&(self->policy.q.lock));
  z_dlink_init(&(self->policy.q.head));
  return(0);
}

static void __xru_destroy (z_cache_t *self, z_memory_t *memory) {
  z_cache_reclaim(self, &(self->policy.q.head));
}

static void __xru_dump (const z_cache_t *self, FILE *stream) {
  z_cache_entry_t *entry;
  fprintf(stream, "%s CACHE\n", self->vpolicy->name);
  z_dlink_for_each_entry(&(self->policy.q.head), entry, z_cache_entry_t, link, {
    fprintf(stream, "%"PRIu64" -> ", entry->map_entry.oid);
  });
  fprintf(stream, "X\n");
}

const z_vtable_cache_policy_t z_cache_policy_lru = {
  .update   = __lru_update,
  .remove   = __xru_remove,
  .reclaim  = __xru_reclaim,

  .create   = __xru_create,
  .destroy  = __xru_destroy,
  .dump     = __xru_dump,
  .name     = "LRU",
};

const z_vtable_cache_policy_t z_cache_policy_mru = {
  .update   = __mru_update,
  .remove   = __xru_remove,
  .reclaim  = __xru_reclaim,

  .create   = __xru_create,
  .destroy  = __xru_destroy,
  .dump     = __xru_dump,
  .name     = "MRU",
};
