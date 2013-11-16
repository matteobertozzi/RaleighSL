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
#include <zcl/hashmap.h>
#include <zcl/string.h>
#include <zcl/math.h>

/* ===========================================================================
 *  PRIVATE Open-HashTable data structures
 */
struct ohtnode {
  uint64_t hash;
  void *   data;
};

/* ===========================================================================
 *  PRIVATE Open-HashTable macros
 */
#define __open_hash_map_buckets(self)                                         \
  ((struct ohtnode *)(self)->buckets)

#define __entry_match(self, node, key_compare, hash, key)                     \
  ((node)->hash == (hash) && !key_compare(self->udata, (node)->data, key))


#define __entry_probe(self, node, hash, __code__)                         \
  do {                                                                    \
    struct ohtnode *buckets = __open_hash_map_buckets(self);              \
    unsigned int count = self->capacity;                                  \
    unsigned int mask = count - 1;                                        \
    node = &(buckets[hash & mask]);                                       \
    do __code__ while (0);                                                \
    do {                                                                  \
      unsigned int shift = z_ilog2(self->capacity);                       \
      uint64_t p = hash;                                                  \
      uint64_t i = hash;                                                  \
      while (count--) {                                                   \
        i = (i << 2) + i + p + 1;                                         \
        node = &(buckets[i & mask]);                                      \
        do __code__ while (0);                                            \
        p >>= shift;                                                      \
      }                                                                   \
    } while (0);                                                          \
  } while (0)

static struct ohtnode *__node_lookup (z_hash_map_t *self,
                                      z_compare_t key_compare,
                                      uint64_t hash,
                                      const void *key)
{
  struct ohtnode *node;
  __entry_probe(self, node, hash, {
    if (__entry_match(self, node, key_compare, hash, key))
      return(node);
  });
  return(NULL);
}

/* ===========================================================================
 *  Open-HashMap plug methods
 */
int __open_hash_map_put (z_hash_map_t *self, uint64_t hash, void *key_value) {
  struct ohtnode *ientry = NULL;
  struct ohtnode *entry;

  __entry_probe(self, entry, hash, {
    if (ientry == NULL && entry->data == NULL) {
      ientry = entry;
    } else if (__entry_match(self, entry, self->key_compare, hash, key_value)) {
      if (self->data_free != NULL && key_value != entry->data)
        self->data_free(self->udata, entry->data);
      entry->data = key_value;
      return(0);
    }
  });

  if (ientry != NULL) {
    ientry->hash = hash;
    ientry->data = key_value;
    self->size++;
    return(0);
  }

  return(1);
}

void * __open_hash_map_get (z_hash_map_t *self,
                            z_compare_t key_compare,
                            uint64_t hash,
                            const void *key)
{
  struct ohtnode *node = __node_lookup(self, key_compare, hash, key);
  return((node != NULL) ? node->data : NULL);
}

void *__open_hash_map_pop (z_hash_map_t *self,
                           z_compare_t key_compare,
                           uint64_t hash,
                           const void *key)
{
  struct ohtnode *node = __node_lookup(self, key_compare, hash, key);
  void *data = node->data;
  node->hash = 0;
  node->data = NULL;
  return(data);
}

void __open_hash_map_close (z_hash_map_t *self) {
  struct ohtnode *bucket;

  if ((bucket = __open_hash_map_buckets(self)) == NULL)
    return;

  z_memory_array_free(z_global_memory(), bucket);
}

int __open_hash_map_resize (z_hash_map_t *self, unsigned int new_size) {
  struct ohtnode *new_buckets;
  struct ohtnode *bucket;
  struct ohtnode *entry;
  unsigned int size;

  new_buckets = z_memory_array_alloc(z_global_memory(), struct ohtnode, new_size);
  if (Z_MALLOC_IS_NULL(new_buckets))
    return(1);

  z_memzero(new_buckets, new_size * sizeof(struct ohtnode));

  size = self->capacity;
  bucket = __open_hash_map_buckets(self);

  self->capacity = new_size;
  self->buckets = new_buckets;
  while (size--) {
    entry = &(bucket[size]);

    if (entry->data != NULL)
      __open_hash_map_put(self, entry->hash, entry->data);
  }

  z_memory_array_free(z_global_memory(), bucket);
  return(0);
}

void __open_hash_map_clear (z_hash_map_t *self) {
  struct ohtnode *buckets;
  unsigned int count;

  count = self->size;
  buckets = __open_hash_map_buckets(self);
  while (count--) {
    struct ohtnode *node = buckets++;
    if (node->data == NULL)
      continue;

    if (self->data_free != NULL)
      self->data_free(self->udata, node->data);

    node->hash = 0;
    node->data = NULL;
  }
}

static void *__open_hash_map_iter_next (z_hash_map_iterator_t *iter,
                                        const z_hash_map_t *map)
{
  struct ohtnode *end = __open_hash_map_buckets(map) + map->capacity;
  struct ohtnode *node = (struct ohtnode *)iter->node;
  while (++node < end) {
    if (node->data != NULL) {
      iter->bucket = node;
      iter->node = node;
      iter->data = node->data;
      return(iter->data);
    }
  }
  iter->bucket = end;
  iter->node = end;
  iter->data = NULL;
  return(NULL);
}

static void *__open_hash_map_iter_prev (z_hash_map_iterator_t *iter,
                                        const z_hash_map_t *map)
{
  struct ohtnode *first = __open_hash_map_buckets(map) - 1;
  struct ohtnode *node = (struct ohtnode *)iter->node;
  while (--node > first) {
    if (node->data != NULL) {
      iter->bucket = node;
      iter->node = node;
      iter->data = node->data;
      return(iter->data);
    }
  }
  iter->bucket = first;
  iter->node = first;
  iter->data = NULL;
  return(NULL);
}

static void *__open_hash_map_iter_begin (z_hash_map_iterator_t *iter,
                                         const z_hash_map_t *map)
{
  struct ohtnode *node = __open_hash_map_buckets(map) - 1;
  iter->bucket = node;
  iter->node = node;
  iter->data = node->data;
  return(__open_hash_map_iter_next(iter, map));
}

static void *__open_hash_map_iter_end (z_hash_map_iterator_t *iter,
                                       const z_hash_map_t *map)
{
  struct ohtnode *node = __open_hash_map_buckets(map) + map->capacity;
  iter->bucket = node;
  iter->node = node;
  iter->data = node->data;
  return(__open_hash_map_iter_prev(iter, map));
}

/* ===========================================================================
 *  Open-HashMap plug vtables
 */
const z_hash_map_plug_t z_open_hash_map = {
  .open       = NULL,
  .close      = __open_hash_map_close,

  .resize     = __open_hash_map_resize,
  .clear      = __open_hash_map_clear,

  .put        = __open_hash_map_put,
  .get        = __open_hash_map_get,
  .pop        = __open_hash_map_pop,

  .iter_begin = __open_hash_map_iter_begin,
  .iter_end   = __open_hash_map_iter_end,
  .iter_next  = __open_hash_map_iter_next,
  .iter_prev  = __open_hash_map_iter_prev,
};