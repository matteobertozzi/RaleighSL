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

#include <zcl/locking.h>
#include <zcl/global.h>
#include <zcl/string.h>
#include <zcl/atomic.h>
#include <zcl/cache.h>
#include <zcl/debug.h>
#include <zcl/hash.h>

/* ============================================================================
 *  PRIVATE Cache data structures
 */
enum entry_states {
  CACHED_ENTRY_IS_NEW,
  CACHED_ENTRY_IS_EVICTED,
  /* LRU */
  CACHED_ENTRY_IS_IN_LRU_QUEUE,
  /* 2Q */
  CACHED_ENTRY_IS_IN_2Q_AM,
  CACHED_ENTRY_IS_IN_2Q_A1IN,
  CACHED_ENTRY_IS_IN_2Q_A1OUT,
};

struct lru_cache {
  z_spinlock_t lock;
  z_dlink_node_t queue;
};

struct q2_cache {
  z_spinlock_t lock;

  uint32_t kin;
  uint32_t kout;

  unsigned int am_size;
  unsigned int a1in_size;
  unsigned int a1out_size;

  z_dlink_node_t am;
  z_dlink_node_t a1in;
  z_dlink_node_t a1out;
};

struct hnode {
  z_rwlock_t lock;
  z_cache_entry_t *entry;
};

struct htable {
  z_rwlock_t lock;
  struct hnode *buckets;
  unsigned int size;
  unsigned int mask;
  unsigned int used;
};

struct z_cache {
  uint64_t hit;
  uint64_t miss;

  unsigned int usage;
  unsigned int capacity;

  struct htable table;

  const z_cache_policy_t *vpolicy;

  union policy {
    struct lru_cache lru;
    struct q2_cache q2;
  } dpolicy;

  z_mem_free_t entry_free;
  z_cache_evict_t entry_evict;
  void *user_data;
};

/* ============================================================================
 *  PRIVATE Hash-Table helper methods
 */
#define __htable_get_bucket(table, oid)                       \
  ((table)->buckets + (z_hash64a(oid) & ((table)->mask)))

static void __htable_resize (struct htable *table, unsigned int required_size) {
  z_memory_t *memory = z_global_memory();
  struct hnode *new_buckets;
  struct hnode *buckets;
  unsigned int new_size;
  unsigned int size;

  new_size = 4;
  while (new_size < required_size)
    new_size <<= 1;

  new_buckets = z_memory_array_alloc(memory, struct hnode, new_size);
  if (Z_MALLOC_IS_NULL(new_buckets))
    return;

  size = new_size * sizeof(struct hnode);
  z_memset(new_buckets, 0, size);

  size = table->size;
  buckets = table->buckets;
  if (buckets != NULL) {
    unsigned int new_mask = new_size - 1;
    unsigned int i;
    for (i = 0; i < size; ++i) {
      z_cache_entry_t *p = buckets[i].entry;
      while (p != NULL) {
        z_cache_entry_t *next;
        struct hnode *node;

        next = p->hash;
        node = new_buckets + (z_hash64a(p->oid) & new_mask);
        p->hash = node->entry;
        node->entry = p;

        p = next;
      }
    }
    z_memory_array_free(memory, buckets);
  }

  table->size = new_size;
  table->mask = new_size - 1;
  table->buckets = new_buckets;
}

static z_cache_entry_t **__htable_find (struct hnode *node, uint64_t oid) {
  z_cache_entry_t **entry = &(node->entry);
  while (*entry && ((*entry)->oid != oid)) {
    entry = &((*entry)->hash);
  }
  return(entry);
}

static z_cache_entry_t **__htable_find_entry (struct hnode *node, uint64_t oid) {
  z_cache_entry_t **entry;
  for (entry = &(node->entry); *entry != NULL; entry = &((*entry)->hash)) {
    if ((*entry)->oid == oid)
      return(entry);
  }
  return(NULL);
}

/* ============================================================================
 *  PRIVATE Hash-Table methods
 */
static int __htable_open (struct htable *table, unsigned int capacity) {
  table->used = 0;
  table->size = 0;
  table->mask = 0;
  table->buckets = NULL;
  z_rwlock_init(&(table->lock));
  __htable_resize(table, capacity);
  return(table->buckets == NULL);
}

static void __htable_close (struct htable *table) {
  z_memory_array_free(z_global_memory(), table->buckets);
}

static z_cache_entry_t *__htable_try_insert (struct htable *table,
                                             z_cache_entry_t *entry)
{
  z_cache_entry_t *old;

  z_read_lock(&(table->lock), z_rwlock, {
    struct hnode *bucket = __htable_get_bucket(table, entry->oid);
    z_write_lock(&(bucket->lock), z_rwlock, {
      z_cache_entry_t **node = __htable_find(bucket, entry->oid);
      if ((old = *node) == NULL) {
        entry->hash = NULL;
        entry->state = CACHED_ENTRY_IS_NEW;
        *node = entry;
        z_atomic_inc(&(entry->refs));
      } else {
        z_atomic_inc(&(old->refs));
      }
    });
  });

  if (old == NULL && z_atomic_inc(&(table->used)) >= (table->size << 1)) {
    z_try_write_lock(&(table->lock), z_rwlock, {
      __htable_resize(table, table->used);
    });
  }

  return(old);
}

static z_cache_entry_t *__htable_lookup (struct htable *table, uint64_t oid) {
  z_cache_entry_t *entry;

  z_read_lock(&(table->lock), z_rwlock, {
    struct hnode *bucket = __htable_get_bucket(table, oid);
    z_read_lock(&(bucket->lock), z_rwlock, {
      z_cache_entry_t **node = __htable_find_entry(bucket, oid);
      if (node != NULL) {
        entry = *node;
        z_atomic_inc(&(entry->refs));
      } else {
        entry = NULL;
      }
    });
  });

  return(entry);
}

static z_cache_entry_t *__htable_remove (struct htable *table, uint64_t oid) {
  z_cache_entry_t *entry;

  z_read_lock(&(table->lock), z_rwlock, {
    struct hnode *bucket = __htable_get_bucket(table, oid);
    z_write_lock(&(bucket->lock), z_rwlock, {
      z_cache_entry_t **node = __htable_find_entry(bucket, oid);
      if (node != NULL) {
        entry = *node;
        *node = entry->hash;
      } else {
        entry = NULL;
      }
    });
  });

  if (entry != NULL)
    z_atomic_dec(&(table->used));
  return(entry);
}

static int __htable_remove_entry (struct htable *table, z_cache_entry_t *entry)
{
  z_cache_entry_t **node;
  int found = 0;

  z_read_lock(&(table->lock), z_rwlock, {
    struct hnode *bucket = __htable_get_bucket(table, entry->oid);
    z_write_lock(&(bucket->lock), z_rwlock, {
      for (node = &(bucket->entry); *node != NULL; node = &((*node)->hash)) {
        if (*node == entry) {
          *node = entry->hash;
          found = 1;
          break;
        }
      }
    });
  });

  if (found)
    z_atomic_dec(&(table->used));
  return(found);
}

/* ===========================================================================
 *  Cache Entry
 */
static int __entry_reclaim (z_cache_t *cache, z_cache_entry_t *entry) {
  if (__htable_remove_entry(&(cache->table), entry)) {
    z_dlink_del(&(entry->cache));
    z_cache_release(cache, entry);
    return(1);
  }
  return(0);
}

void z_cache_entry_init (z_cache_entry_t *entry, uint64_t oid) {
  entry->hash = NULL;
  z_dlink_init(&(entry->cache));

  entry->oid = oid;

  entry->refs = 1;
  entry->state = CACHED_ENTRY_IS_NEW;
}

/* ============================================================================
 *  LRU
 */
static void __lru_reclaim (z_cache_t *cache, struct lru_cache *lru) {
  z_dlink_node_t *tail = lru->queue.prev;
  size_t usage = cache->usage;
  while (usage > cache->capacity && tail != &(lru->queue)) {
    z_cache_entry_t *evicted = z_dlink_entry(tail, z_cache_entry_t, cache);

    if (cache->entry_evict != NULL &&
        !cache->entry_evict(cache->user_data, usage, evicted))
    {
      break;
    }

    tail = tail->prev;
    usage--;
    __entry_reclaim(cache, evicted);
  }
}

static void __lru_insert (z_cache_t *cache,
                          struct lru_cache *lru,
                          z_cache_entry_t *entry)
{
  if (entry->state == CACHED_ENTRY_IS_NEW) {
    /* Insert to the head of LRU */
    z_dlink_add(&(lru->queue), &(entry->cache));
    entry->state = CACHED_ENTRY_IS_IN_LRU_QUEUE;
  } else {
    /* Move to the head of LRU */
    z_dlink_move(&(lru->queue), &(entry->cache));
  }
}

static void __lru_remove (z_cache_t *cache,
                          struct lru_cache *lru,
                          z_cache_entry_t *entry)
{
  z_dlink_del(&(entry->cache));
}

static void __lru_init (z_cache_t *cache, struct lru_cache *lru) {
  z_dlink_init(&(lru->queue));
}

static void __policy_lru_update (z_cache_t *cache, z_cache_entry_t *entry) {
  struct lru_cache *lru = &(cache->dpolicy.lru);
  z_lock(&(lru->lock), z_spin, {
    if (entry->state != CACHED_ENTRY_IS_EVICTED)
      __lru_insert(cache, lru, entry);
  });
}

static void __policy_lru_remove (z_cache_t *cache, z_cache_entry_t *entry) {
  struct lru_cache *lru = &(cache->dpolicy.lru);
  z_lock(&(lru->lock), z_spin, {
    __lru_remove(cache, lru, entry);
  });
  z_cache_release(cache, entry);
}

static void __policy_lru_reclaim (z_cache_t *cache) {
  struct lru_cache *lru = &(cache->dpolicy.lru);
  z_lock(&(lru->lock), z_spin, {
    __lru_reclaim(cache, lru);
  });
}

static void __policy_lru_init (z_cache_t *cache) {
  struct lru_cache *lru = &(cache->dpolicy.lru);
  z_spin_alloc(&(lru->lock));
  __lru_init(cache, lru);
}

static void __policy_lru_destroy (z_cache_t *cache) {
  struct lru_cache *lru = &(cache->dpolicy.lru);
  z_cache_entry_t *entry;
  z_spin_free(&(lru->lock));
  z_dlink_for_each_safe_entry(&(lru->queue), entry, z_cache_entry_t, cache, {
    z_cache_release(cache, entry);
  });
  __lru_init(cache, lru);
}

static void __policy_lru_dump (FILE *stream, z_cache_t *cache) {
  struct lru_cache *lru = &(cache->dpolicy.lru);
  z_cache_entry_t *entry;
  fprintf(stream, "LRU CACHE\n");
  z_dlink_for_each_entry(&(lru->queue), entry, z_cache_entry_t, cache, {
    fprintf(stream, "%lu -> ", entry->oid);
  });
  fprintf(stream, "X\n");
}

static const z_cache_policy_t __policy_lru = {
  .insert  = __policy_lru_update,
  .update  = __policy_lru_update,
  .remove  = __policy_lru_remove,
  .reclaim = __policy_lru_reclaim,
  .init    = __policy_lru_init,
  .destroy = __policy_lru_destroy,
  .dump    = __policy_lru_dump,
  .type    = Z_CACHE_LRU,
};

/* ============================================================================
 *  2Q: A Low Overhead High Performance Buffer Management Replacement
 *  Algorithm", by Theodore Johnson and Dennis Shasha.
 *  http://www.vldb.org/conf/1994/P439.PDF
 */
static void __q2_reclaim (z_cache_t *cache, struct q2_cache *q2) {
  z_cache_entry_t *evicted;
  while (cache->usage > cache->capacity && q2->a1in_size > 0) {
    if (q2->a1in_size > q2->kin) {
      // page out the tail of a1in, and add it to the head of a1out
      evicted = z_dlink_entry(q2->a1in.prev, z_cache_entry_t, cache);
      z_dlink_move(&(q2->a1out), &(evicted->cache));
      q2->a1out_size++;
      q2->a1in_size--;
      evicted->state = CACHED_ENTRY_IS_IN_2Q_A1OUT;
      if (q2->a1out_size > q2->kout) {
        // remove identifier of Z from the tail of A1out
        evicted = z_dlink_entry(q2->a1out.prev, z_cache_entry_t, cache);
        q2->a1out_size--;
        if (!__entry_reclaim(cache, evicted))
          break;
      }
    } else if (q2->am_size > 0) {
      // page out the tail of AM.
      // do not put it on A1out; it hasn't been accessed for a while
      evicted = z_dlink_entry(q2->am.prev, z_cache_entry_t, cache);
      q2->am_size--;
      if (!__entry_reclaim(cache, evicted))
        break;
    }
  }
}

static void __q2_insert (z_cache_t *cache,
                         struct q2_cache *q2,
                         z_cache_entry_t *entry)
{
  if (entry->state == CACHED_ENTRY_IS_IN_2Q_AM) {
    /* move to head of am */
    z_dlink_move(&(q2->am), &(entry->cache));
  } else if (entry->state == CACHED_ENTRY_IS_IN_2Q_A1OUT) {
    /* add to head of am */
    z_dlink_move(&(q2->am), &(entry->cache));
    q2->am_size++;
    q2->a1out_size--;
    entry->state = CACHED_ENTRY_IS_IN_2Q_AM;
  } else if (entry->state == CACHED_ENTRY_IS_IN_2Q_A1IN) {
    /* move to head of a1in */
    z_dlink_move(&(q2->a1in), &(entry->cache));
  } else /* if (entry->state == ENTRY_IS_NEW) */ {
    /* add to head of a1in */
    z_dlink_add(&(q2->a1in), &(entry->cache));
    q2->a1in_size++;
    entry->state = CACHED_ENTRY_IS_IN_2Q_A1IN;
  }
}

static void __q2_remove (z_cache_t *cache,
                         struct q2_cache *q2,
                         z_cache_entry_t *entry)
{
  switch (entry->state) {
    case CACHED_ENTRY_IS_IN_2Q_AM:
      q2->am_size--;
      break;
    case CACHED_ENTRY_IS_IN_2Q_A1IN:
      q2->a1in_size--;
      break;
    case CACHED_ENTRY_IS_IN_2Q_A1OUT:
      q2->a1out_size--;
      break;
  }

  z_dlink_del(&(entry->cache));
}

static void __q2_init (z_cache_t *cache, struct q2_cache *q2) {
  q2->kin = 1;
  q2->kout = cache->capacity / 2;

  q2->am_size = 0;
  q2->a1in_size = 0;
  q2->a1out_size = 0;

  z_dlink_init(&(q2->am));
  z_dlink_init(&(q2->a1in));
  z_dlink_init(&(q2->a1out));
}

static void __policy_2q_update (z_cache_t *cache, z_cache_entry_t *entry) {
  struct q2_cache *q2 = &(cache->dpolicy.q2);
  z_lock(&(q2->lock), z_spin, {
    if (entry->state != CACHED_ENTRY_IS_EVICTED)
      __q2_insert(cache, q2, entry);
  });
}

static void __policy_2q_remove (z_cache_t *cache, z_cache_entry_t *entry) {
  struct q2_cache *q2 = &(cache->dpolicy.q2);
  z_lock(&(q2->lock), z_spin, {
    __q2_remove(cache, q2, entry);
  });
  z_cache_release(cache, entry);
}

static void __policy_2q_reclaim (z_cache_t *cache) {
  struct q2_cache *q2 = &(cache->dpolicy.q2);
  z_lock(&(q2->lock), z_spin, {
    __q2_reclaim(cache, q2);
  });
}

static void __policy_2q_init (z_cache_t *cache) {
  struct q2_cache *q2 = &(cache->dpolicy.q2);
  z_spin_alloc(&(q2->lock));
  __q2_init(cache, q2);
}

static void __policy_2q_destroy (z_cache_t *cache) {
  struct q2_cache *q2 = &(cache->dpolicy.q2);
  z_cache_entry_t *entry;
  z_spin_free(&(q2->lock));
  z_dlink_for_each_safe_entry(&(q2->am), entry, z_cache_entry_t, cache, {
    z_cache_release(cache, entry);
  });
  z_dlink_init(&(q2->am));
  z_dlink_for_each_safe_entry(&(q2->a1out), entry, z_cache_entry_t, cache, {
    z_cache_release(cache, entry);
  });
  z_dlink_init(&(q2->a1out));
  z_dlink_for_each_safe_entry(&(q2->a1in), entry, z_cache_entry_t, cache, {
    z_cache_release(cache, entry);
  });
  z_dlink_init(&(q2->a1in));
}

static void __policy_2q_dump (FILE *stream, z_cache_t *cache) {
  struct q2_cache *q2 = &(cache->dpolicy.q2);
  z_cache_entry_t *entry;
  fprintf(stream, "2Q CACHE\n");
  fprintf(stream, " - am_size:    %u\n", q2->am_size);
  fprintf(stream, " - a1out_size: %u\n", q2->a1out_size);
  fprintf(stream, " - a1in_size:  %u\n", q2->a1in_size);
  fprintf(stream, " - am: ");
  z_dlink_for_each_entry(&(q2->am), entry, z_cache_entry_t, cache, {
    fprintf(stream, "%lu -> ", entry->oid);
  });
  fprintf(stream, "X\n");
  fprintf(stream, " - a1out: ");
  z_dlink_for_each_entry(&(q2->a1out), entry, z_cache_entry_t, cache, {
    fprintf(stream, "%lu -> ", entry->oid);
  });
  fprintf(stream, "X\n");
  fprintf(stream, " - a1in: ");
  z_dlink_for_each_entry(&(q2->a1in), entry, z_cache_entry_t, cache, {
    fprintf(stream, "%lu -> ", entry->oid);
  });
  fprintf(stream, "X\n");
}

static const z_cache_policy_t __policy_2q = {
  .insert  = __policy_2q_update,
  .update  = __policy_2q_update,
  .remove  = __policy_2q_remove,
  .reclaim = __policy_2q_reclaim,
  .init    = __policy_2q_init,
  .destroy = __policy_2q_destroy,
  .dump    = __policy_2q_dump,
  .type    = Z_CACHE_2Q,
};

/* ===========================================================================
 *  Cache
 */
z_cache_t *z_cache_alloc (z_cache_type_t type,
                          unsigned int capacity,
                          z_mem_free_t entry_free,
                          z_cache_evict_t entry_evict,
                          void *user_data)
{
  z_cache_t *cache;

  cache = z_memory_struct_alloc(z_global_memory(), z_cache_t);
  if (Z_MALLOC_IS_NULL(cache))
    return(NULL);

  cache->hit = 0;
  cache->miss = 0;
  cache->capacity = capacity;
  cache->usage = 0;

  cache->entry_free = entry_free;
  cache->entry_evict = entry_evict;
  cache->user_data = user_data;

  __htable_open(&(cache->table), capacity);

  switch (type) {
    case Z_CACHE_LRU:
      cache->vpolicy = &__policy_lru;
      break;
    case Z_CACHE_2Q:
      cache->vpolicy = &__policy_2q;
      break;
  }

  cache->vpolicy->init(cache);
  return(cache);
}

void z_cache_free (z_cache_t *cache) {
  cache->vpolicy->destroy(cache);
  __htable_close(&(cache->table));
  z_memory_struct_free(z_global_memory(), z_cache_t, cache);
}

void z_cache_release (z_cache_t *cache, z_cache_entry_t *entry) {
  Z_ASSERT(entry->refs > 0, "Entry already unreferenced");
  if (z_atomic_dec(&(entry->refs)) == 0) {
    Z_LOG_TRACE("Cache Release %u", entry->oid);
    z_atomic_dec(&(cache->usage));
    if (cache->entry_free != NULL)
      cache->entry_free(cache->user_data, entry);
  }
}

z_cache_entry_t *z_cache_try_insert (z_cache_t *cache, z_cache_entry_t *entry) {
  z_cache_entry_t *old;

  old = __htable_try_insert(&(cache->table), entry);
  if (old != NULL) {
    z_atomic_inc(&(cache->hit));
    cache->vpolicy->update(cache, entry);
  } else {
    cache->vpolicy->insert(cache, entry);
    z_atomic_inc(&(cache->miss));
    z_atomic_inc(&(cache->usage));
    cache->vpolicy->reclaim(cache);
  }

  return(old);
}

z_cache_entry_t *z_cache_lookup (z_cache_t *cache, uint64_t oid) {
  z_cache_entry_t *entry;

  if ((entry = __htable_lookup(&(cache->table), oid)) != NULL) {
    cache->vpolicy->update(cache, entry);
    z_atomic_inc(&(cache->hit));
  } else {
    z_atomic_inc(&(cache->miss));
  }
  return(entry);
}

z_cache_entry_t *z_cache_remove (z_cache_t *cache, uint64_t oid) {
  z_cache_entry_t *entry;
  if ((entry = __htable_remove(&(cache->table), oid)) != NULL) {
    cache->vpolicy->remove(cache, entry);
    entry->state = CACHED_ENTRY_IS_EVICTED;
  }
  return(entry);
}

void z_cache_dump (FILE *stream, z_cache_t *cache) {
  cache->vpolicy->dump(stream, cache);
  fprintf(stream,
          " - hit %lu miss %lu (%.2f hit)\n"
          " - htable used %u size %u\n"
          " - usage %u capacity %u\n\n",
          cache->hit, cache->miss, (double)cache->hit / (cache->hit + cache->miss),
          cache->table.used, cache->table.size,
          cache->usage, cache->capacity);
}
