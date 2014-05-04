#include <zcl/global.h>
#include <zcl/memory.h>
#include <zcl/ticket.h>
#include <zcl/rwlock.h>
#include <zcl/hash.h>
#include <zcl/chmap.h>

struct bucket {
  z_rwlock_t lock;
  z_chmap_entry_t *entry;
};

struct segment {
  struct bucket *buckets;
};

struct z_chmap {
  z_ticket_t lock;
  uint32_t   width;
  uint32_t   wmask;
  uint32_t   used;
  uint32_t   nsegments;
  struct segment segments[1];
};

/* ============================================================================
 *  PRIVATE Bucket macros
 */
#define __bucket_find_slot(bucket, entry_id) ({                       \
  z_chmap_entry_t **__entry = &((bucket)->entry);                     \
  while (*__entry && ((*__entry)->oid < (entry_id))) {                \
    __entry = &((*__entry)->hash);                                    \
  }                                                                   \
  __entry;                                                            \
})

/* ============================================================================
 *  PRIVATE Segment methods
 */
#define __segment_get_bucket(map, segment, hash)                       \
  ((segment)->buckets + (hash & ((map)->wmask)))

static z_chmap_entry_t *__segment_try_insert (z_chmap_t *map,
                                             struct segment *segment,
                                             z_chmap_entry_t *entry,
                                             uint64_t hash)
{
  z_chmap_entry_t **node;
  struct bucket *bucket;
  z_chmap_entry_t *old;

  bucket = __segment_get_bucket(map, segment, hash);
  z_rwlock_write_lock(&(bucket->lock));
  node = __bucket_find_slot(bucket, entry->oid);
  if (*node != NULL && (*node)->oid == entry->oid) {
    old = *node;
    z_atomic_inc(&(old->refs));
  } else {
    old = NULL;
    entry->hash = *node;
    *node = entry;
    z_atomic_inc(&(entry->refs));
  }
  z_rwlock_write_unlock(&(bucket->lock));

  return(old);
}

static z_chmap_entry_t *__segment_remove (z_chmap_t *map,
                                          struct segment *segment,
                                          uint64_t oid,
                                          uint64_t hash)
{
  z_chmap_entry_t *entry;
  z_chmap_entry_t **node;
  struct bucket *bucket;

  bucket = __segment_get_bucket(map, segment, hash);
  z_rwlock_write_lock(&(bucket->lock));
  node = __bucket_find_slot(bucket, oid);
  if (*node != NULL && (*node)->oid == oid) {
    entry = *node;
    *node = entry->hash;
  } else {
    entry = NULL;
  }
  z_rwlock_write_unlock(&(bucket->lock));

  return(entry);
}

static int __segment_remove_entry (z_chmap_t *map,
                                   struct segment *segment,
                                   z_chmap_entry_t *entry,
                                   uint64_t hash)
{
  z_chmap_entry_t **node;
  struct bucket *bucket;
  int found = 0;

  bucket = __segment_get_bucket(map, segment, hash);
  z_rwlock_write_lock(&(bucket->lock));
  for (node = &(bucket->entry); *node != NULL; node = &((*node)->hash)) {
    if (*node == entry) {
      *node = entry->hash;
      found = 1;
      break;
    }
  }
  z_rwlock_write_unlock(&(bucket->lock));

  return(found);
}

static z_chmap_entry_t *__segment_lookup (z_chmap_t *map,
                                         struct segment *segment,
                                         uint64_t oid,
                                         uint64_t hash)
{
  z_chmap_entry_t *entry;
  z_chmap_entry_t **node;
  struct bucket *bucket;

  bucket = __segment_get_bucket(map, segment, hash);
  z_rwlock_read_lock(&(bucket->lock));
  node = __bucket_find_slot(bucket, oid);
  if (*node != NULL && (*node)->oid == oid) {
    entry = *node;
    z_atomic_inc(&(entry->refs));
  } else {
    entry = NULL;
  }
  z_rwlock_read_unlock(&(bucket->lock));

  return(entry);
}

static int __segment_create (z_chmap_t *map, z_memory_t *memory) {
  struct bucket *buckets;
  uint32_t i;

  fprintf(stderr, "ALLOC SEGMENT bucket %lu\n", sizeof(struct bucket));

  buckets = z_memory_array_alloc(memory, struct bucket, map->width);
  if (Z_MALLOC_IS_NULL(buckets))
    return(1);

  for (i = 0; i < map->width; ++i) {
    struct bucket *pbucket = &(buckets[i]);
    z_rwlock_init(&(pbucket->lock));
    pbucket->entry = NULL;
  }

  map->segments[0].buckets = buckets;
  map->nsegments = 1;
  return(0);
}

static void __segment_destroy (z_chmap_t *map, z_memory_t *memory) {
  z_memory_array_free(memory, map->segments[0].buckets);
  map->nsegments = 0;
}

/* ============================================================================
 *  PUBLIC Concurrent-HashMap Entry methods
 */
void z_chmap_entry_init (z_chmap_entry_t *entry, uint64_t oid) {
  entry->hash = NULL;
  entry->oid = oid;
  entry->refs = 1;
  entry->ustate = 0;
  entry->uflags8 = 0;
  entry->uweight = 0;
}

/* ============================================================================
 *  PUBLIC Concurrent-HashMap methods
 */
z_chmap_t *z_chmap_create (z_memory_t *memory, uint32_t size) {
  z_chmap_t *map;

  map = z_memory_struct_alloc(memory, z_chmap_t);
  if (Z_MALLOC_IS_NULL(map))
    return(NULL);

  z_ticket_init(&(map->lock));
  map->width = z_align_up(size, 8);
  map->wmask = map->width - 1;
  map->used = 0;
  map->nsegments = 0;

  if (__segment_create(map, memory)) {
    z_memory_struct_free(memory, z_chmap_t, map);
    return(NULL);
  }

  return(map);
}

void z_chmap_destroy (z_memory_t *memory, z_chmap_t *map) {
  while (map->nsegments > 0) {
    __segment_destroy(map, memory);
  }
  z_memory_struct_free(memory, z_chmap_t, map);
}

z_chmap_entry_t *z_chmap_try_insert (z_chmap_t *map, z_chmap_entry_t *entry) {
  z_chmap_entry_t *old;
  uint64_t hash;

  hash = z_hash64a(entry->oid);
  old = __segment_try_insert(map, &(map->segments[0]), entry, hash);
  if (old == NULL) {
    z_atomic_inc(&(map->used));
  }
  return(old);
}

z_chmap_entry_t *z_chmap_remove (z_chmap_t *map, uint64_t oid) {
  z_chmap_entry_t *entry;
  uint64_t hash;

  hash = z_hash64a(oid);
  entry = __segment_remove(map, &(map->segments[0]), oid, hash);
  if (entry != NULL) {
    z_atomic_dec(&(map->used));
  }
  return(entry);
}

int z_chmap_remove_entry (z_chmap_t *map, z_chmap_entry_t *entry) {
  uint64_t hash;
  int found;

  hash = z_hash64a(entry->oid);
  found = __segment_remove_entry(map, &(map->segments[0]), entry, hash);
  if (found) {
    z_atomic_dec(&(map->used));
  }
  return(0);
}

z_chmap_entry_t *z_chmap_lookup (z_chmap_t *map, uint64_t oid) {
  uint64_t hash = z_hash64a(oid);
  return(__segment_lookup(map, &(map->segments[0]), oid, hash));
}