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

#include <zcl/skiplist.h>
#include <zcl/global.h>
#include <zcl/bucket.h>
#include <zcl/debug.h>
#include <zcl/dlink.h>
#include <zcl/bytes.h>
#include <zcl/time.h>

#include "sset.h"

#define RALEIGHSL_SSET(x)                 Z_CAST(raleighsl_sset_t, x)
#define __SSET_ENTRY(x)                   Z_CAST(struct sset_entry, x)
#define __SSET_NODE(x)                    Z_CAST(struct sset_node, x)
#define __SSET_BLOCK(x)                   Z_CAST(struct sset_block, x)

/* TODO: REPLACE: Test Values */
#define __SSET_BLOCK_SIZE                 (8 << 10)
#define __SSET_BLOCK_MERGE_SIZE           (__SSET_BLOCK_SIZE - (__SSET_BLOCK_SIZE >> 2))
#define __SSET_SYNC_SIZE                  (1 << 10)

typedef struct raleighsl_sset {
  z_skip_list_t skip_list;
  z_skip_list_t txn_locks;
  z_dlink_node_t dirtyq;
} raleighsl_sset_t;

struct sset_block {
  z_dlink_node_t blkseq;
  z_bytes_ref_t first_key;
  z_bytes_ref_t last_key;
  uint32_t refs;
  uint8_t data[__SSET_BLOCK_SIZE];
};

struct sset_node {
  z_bytes_ref_t key;
  z_skip_list_t skip_list;
  struct sset_block *block;
  unsigned int bufsize;
  z_dlink_node_t dirtyq;
};

struct sset_entry {
  z_bytes_ref_t key;
  z_bytes_ref_t value;
  struct sset_node *node;
};

enum sset_txn_type {
  SSET_TXN_INSERT,
  SSET_TXN_UPDATE,
  SSET_TXN_REMOVE
};

struct sset_txn {
  struct sset_entry *entry;
  uint64_t txn_id;
  enum sset_txn_type type;
};

/* ============================================================================
 *  PRIVATE  SSet Entry methods
 */
static int __sset_entry_compare (void *udata, const void *a, const void *b) {
  const struct sset_entry *ea = (const struct sset_entry *)a;
  const struct sset_entry *eb = (const struct sset_entry *)b;
  return(z_bytes_ref_compare(&(ea->key), &(eb->key)));
}

static int __sset_entry_key_compare (void *udata, const void *a, const void *key) {
  const struct sset_entry *entry = (const struct sset_entry *)a;
  return(z_bytes_ref_compare(&(entry->key), Z_CONST_BYTES_REF(key)));
}

static void __sset_entry_free (void *udata, void *obj) {
  struct sset_entry *entry = __SSET_ENTRY(obj);
  z_bytes_ref_release(&(entry->key));
  z_bytes_ref_release(&(entry->value));
  z_memory_struct_free(z_global_memory(), struct sset_entry, entry);
}

static struct sset_entry *__sset_entry_alloc (raleighsl_sset_t *sset,
                                              const z_bytes_ref_t *key,
                                              const z_bytes_ref_t *value,
                                              struct sset_node *node)
{
  z_memory_t *memory = z_global_memory();
  struct sset_entry *entry;

  entry = z_memory_struct_alloc(memory, struct sset_entry);
  if (Z_MALLOC_IS_NULL(entry))
    return(NULL);

  z_bytes_ref_acquire(&(entry->key), key);
  z_bytes_ref_acquire(&(entry->value), value);
  entry->node = node;
  return(entry);
}

static void __sset_entry_set_value (raleighsl_sset_t *sset,
                                    struct sset_entry *entry,
                                    const z_bytes_ref_t *value)
{
  z_bytes_ref_release(&(entry->value));
  z_bytes_ref_acquire(&(entry->value), value);
}

/* ============================================================================
 *  SSet Entry Skip-List Iterator
 */
struct sset_entry_map_iterator {
  __Z_MAP_ITERABLE__
  z_skip_list_iterator_t iter;
  z_map_entry_t entry;
};

static const z_vtable_map_iterator_t __sset_entry_map_iterator;

static void __sset_entry_to_map (struct sset_entry *entry, z_map_entry_t *map) {
  if (Z_LIKELY(entry != NULL)) {
    z_byte_slice_copy(&(map->key), &(entry->key.slice));
    z_byte_slice_copy(&(map->value), &(entry->value.slice));
  }
}

static int __sset_entry_map_open (void *self, z_skip_list_t *skip_list) {
  struct sset_entry_map_iterator *iter = (struct sset_entry_map_iterator *)self;
  Z_MAP_ITERATOR_INIT(self, &__sset_entry_map_iterator, &z_vtable_bytes_refs, NULL);
  z_iterator_open(&(iter->iter), skip_list);
  return(0);
}

static int __sset_entry_map_begin (void *self) {
  struct sset_entry_map_iterator *iter = (struct sset_entry_map_iterator *)self;
  struct sset_entry *entry = __SSET_ENTRY(z_iterator_begin(&(iter->iter)));
  __sset_entry_to_map(entry, &(iter->entry));
  Z_MAP_ITERATOR_HEAD(self).object = entry;
  return(entry != NULL);
}

static int __sset_entry_map_next (void *self) {
  struct sset_entry_map_iterator *iter = (struct sset_entry_map_iterator *)self;
  struct sset_entry *entry = __SSET_ENTRY(z_iterator_next(&(iter->iter)));
  __sset_entry_to_map(entry, &(iter->entry));
  Z_MAP_ITERATOR_HEAD(self).object = entry;
  return(entry != NULL);
}

static int __sset_entry_map_seek (void *self, const z_byte_slice_t *key) {
  struct sset_entry_map_iterator *iter = (struct sset_entry_map_iterator *)self;
  struct sset_entry *entry = __SSET_ENTRY(z_iterator_seek_le(&(iter->iter), __sset_entry_key_compare, key));
  __sset_entry_to_map(entry, &(iter->entry));
  Z_MAP_ITERATOR_HEAD(self).object = entry;
  return(entry != NULL);
}

const z_map_entry_t *__sset_entry_map_current (const void *self) {
  const struct sset_entry_map_iterator *iter = (const struct sset_entry_map_iterator *)self;
  struct sset_entry *entry = __SSET_ENTRY(Z_MAP_ITERATOR_HEAD(iter).object);
  return((entry != NULL) ? &(iter->entry) : NULL);
}

static void __sset_entry_map_get_refs (const void *self,
                                       z_bytes_ref_t *key,
                                       z_bytes_ref_t *value)
{
  const struct sset_entry_map_iterator *iter = (const struct sset_entry_map_iterator *)self;
  struct sset_entry *entry = __SSET_ENTRY(Z_MAP_ITERATOR_HEAD(iter).object);

  if (Z_UNLIKELY(entry == NULL))
    return;

  if (key != NULL) {
    z_bytes_ref_acquire(key, &(entry->key));
  }

  if (value != NULL) {
    z_bytes_ref_acquire(value, &(entry->value));
  }
}

static const z_vtable_map_iterator_t __sset_entry_map_iterator = {
  .begin    = __sset_entry_map_begin,
  .next     = __sset_entry_map_next,
  .seek     = __sset_entry_map_seek,
  .current  = __sset_entry_map_current,
  .get_refs = __sset_entry_map_get_refs,
};

/* ============================================================================
 *  SSet Block
 */
static struct sset_block *__sset_block_alloc (void) {
  struct sset_block *block;

  block = z_memory_struct_alloc(z_global_memory(), struct sset_block);
  if (Z_MALLOC_IS_NULL(block))
    return(NULL);

  z_dlink_init(&(block->blkseq));
  z_bytes_ref_reset(&(block->first_key));
  z_bytes_ref_reset(&(block->last_key));
  block->refs = 1;
  return(block);
}

static void __sset_block_free (struct sset_block *self) {
  if (self == NULL || z_atomic_dec(&(self->refs)) > 0)
    return;
  z_bytes_ref_release(&(self->first_key));
  z_bytes_ref_release(&(self->last_key));
  z_memory_struct_free(z_global_memory(), struct sset_block, self);
}

static void __sset_block_inc_ref (void *object) {
  z_atomic_inc(&(__SSET_BLOCK(object)->refs));
}

static void __sset_block_dec_ref (void *object) {
  __sset_block_free(__SSET_BLOCK(object));
}

static const z_vtable_refs_t __sset_block_vtable_refs = {
  .inc_ref = __sset_block_inc_ref,
  .dec_ref = __sset_block_dec_ref,
};

static struct sset_block *__sset_block_create (const z_byte_slice_t *key) {
  struct sset_block *block;
  z_bytes_t *first_key;

  block = __sset_block_alloc();
  if (Z_MALLOC_IS_NULL(block))
    return(NULL);

  first_key = z_bytes_from_byte_slice(key);
  z_bytes_ref_set(&(block->first_key), &(first_key->slice), &z_vtable_bytes_refs, first_key);
  z_bucket_variable.create(block->data, sizeof(block->data));
  return(block);
}

static void __sset_block_finalize (struct sset_block *self, const z_byte_slice_t *key) {
  z_bytes_t *last_key;

  last_key = z_bytes_from_byte_slice(key);
  z_bytes_ref_set(&(self->last_key), &(last_key->slice), &z_vtable_bytes_refs, last_key);

  z_bucket_variable.finalize(self->data);
}

static void __sset_fill_bucket_entry (z_bucket_entry_t *item,
                                      const z_byte_slice_t *key,
                                      const z_byte_slice_t *value,
                                      const z_byte_slice_t *kprev)
{
  item->kprefix = z_memshared(kprev->data, kprev->size,
                              key->data, key->size);
  z_byte_slice_set(&(item->key),
                   key->data + item->kprefix,
                   key->size - item->kprefix);
  z_byte_slice_copy(&(item->value), value);
}

static int __sset_block_add (struct sset_block *self,
                             const z_byte_slice_t *key,
                             const z_byte_slice_t *value,
                             const z_byte_slice_t *kprev)
{
  z_bucket_entry_t item;
  __sset_fill_bucket_entry(&item, key, value, kprev);
  return(z_bucket_variable.append(self->data, &item));
}

#define __sset_block_add_entry(self, entry, kprev)                            \
  __sset_block_add(self, z_bytes_slice(&(entry->key)),                        \
                   z_bytes_slice(&(entry->value)), kprev)

static int __sset_block_lookup (struct sset_block *self,
                                const z_bytes_ref_t *key,
                                z_bytes_ref_t *value)
{
  z_byte_slice_t val;
  int cmp = z_bucket_search(&z_bucket_variable, self->data,
                            z_bytes_slice(key), &val);
  if (!cmp && value != NULL) {
    z_atomic_inc(&(self->refs));
    z_bytes_ref_set(value, &val, &__sset_block_vtable_refs, self);
  }
  return(cmp);
}

/* ============================================================================
 *  SSet Node
 */
static struct sset_node *__sset_node_alloc (raleighsl_sset_t *sset,
                                            struct sset_block *block)
{
  struct sset_node *node;

  node = z_memory_struct_alloc(z_global_memory(), struct sset_node);
  if (Z_MALLOC_IS_NULL(node))
    return(NULL);

  if (z_skip_list_alloc(&(node->skip_list),
                        __sset_entry_compare, __sset_entry_free,
                        sset, (size_t)node) == NULL)
  {

    z_memory_struct_free(z_global_memory(), struct sset_node, node);
    return(NULL);
  }

  if (block == NULL) {
    z_bytes_ref_reset(&(node->key));
    node->block = NULL;
  } else {
    z_bytes_ref_acquire(&(node->key), &(block->first_key));
    node->block = block;
  }

  node->bufsize = 0;
  z_dlink_init(&(node->dirtyq));
  return(node);
}

static void __sset_node_free (void *udata, void *obj) {
  struct sset_node *node = __SSET_NODE(obj);
  z_bytes_ref_release(&(node->key));
  __sset_block_free(node->block);
  z_skip_list_free(&(node->skip_list));
  z_memory_struct_free(z_global_memory(), struct sset_node, node);
}

static int __sset_node_compare (void *udata, const void *a, const void *b) {
  const struct sset_node *ea = (const struct sset_node *)a;
  const struct sset_node *eb = (const struct sset_node *)b;
  return(z_bytes_ref_compare(&(ea->key), &(eb->key)));
}

static int __sset_node_key_compare (void *udata, const void *a, const void *key) {
  const struct sset_node *node = (const struct sset_node *)a;
  return(z_bytes_ref_compare(&(node->key), Z_CONST_BYTES_REF(key)));
}

#define __sset_node_lookup(sset, key)                                          \
  __SSET_NODE(z_skip_list_less_eq_custom(&((sset)->skip_list),                 \
                                         __sset_node_key_compare, key, NULL))

static int __sset_node_search (struct sset_node *node,
                               const z_bytes_ref_t *key,
                               z_bytes_ref_t *value)
{
  struct sset_entry *entry;

  /* Try to lookup the key in the In-Memory buffer */
  entry = __SSET_ENTRY(z_skip_list_get_custom(&(node->skip_list),
                                              __sset_entry_key_compare, key));
  if (entry != NULL) {
    if (value != NULL)
      z_bytes_ref_acquire(value, &(entry->value));
    return(1);
  }

  /* Try to lookup in the block */
  if (node->block != NULL)
    return(!__sset_block_lookup(node->block, key, value));

  /* Not found */
  return(0);
}

#if 1
static void __sset_block_debug (struct sset_block *block) {
  char first_key[128];
  char last_key[128];

  z_byte_slice_memcpy(first_key, z_bytes_ref_slice(&(block->first_key)));
  first_key[z_bytes_ref_size(&(block->first_key))] = 0;

  z_byte_slice_memcpy(last_key, z_bytes_ref_slice(&(block->last_key)));
  last_key[z_bytes_ref_size(&(block->last_key))] = 0;

  Z_LOG_TRACE("Block First-Key %s Last-Key %s Refs %"PRIu32" Avail %"PRIu32,
              first_key, last_key, block->refs,
              z_bucket_variable.available(block->data));
}
#endif
static void __sset_node_sync_merge (raleighsl_sset_t *sset,
                                    struct sset_node *node,
                                    z_dlink_node_t *node_blkseq,
                                    z_dlink_node_t *blkseq)
{
  struct sset_block *block;

  /* If the last-key < first-key or first-key > last-key don't merge */
  z_dlink_for_each_safe_entry(node_blkseq, block, struct sset_block, blkseq, {
    if (z_bucket_variable.available(block->data) < __SSET_BLOCK_MERGE_SIZE &&
        (z_bytes_ref_compare(&(block->last_key), &(node->block->first_key)) < 0 ||
         z_bytes_ref_compare(&(block->first_key), &(node->block->last_key)) > 0))
    {
      z_dlink_move_tail(blkseq, &(block->blkseq));
    }
  });

  if (z_dlink_is_empty(node_blkseq)) {
    z_dlink_move_tail(blkseq, &(node->block->blkseq));
  } else {
    z_bucket_iterator_t iters[16];
    const z_map_entry_t *entry;
    uint8_t last_key_buf[128];
    z_byte_slice_t last_key;
    z_map_merger_t merger;
    uint8_t niters = 0;

    z_map_merger_open(&merger);

    z_dlink_add(node_blkseq, &(node->block->blkseq));
    z_dlink_for_each_entry(node_blkseq, block, struct sset_block, blkseq, {
      Z_ASSERT(niters < 16, "Max number of merge-nodes is 16 got %"PRIu8, niters);
      z_bucket_iterator_open(&(iters[niters]), &z_bucket_variable, block->data,
                             &__sset_block_vtable_refs, block);
      z_map_iterator_begin(&(iters[niters]));
      z_map_merger_add(&merger, Z_MAP_ITERATOR(&(iters[niters])));
      niters++;
    });

    entry = z_map_merger_next(&merger);
    z_byte_slice_clear(&last_key);
    while (entry != NULL) {
      struct sset_block *block;
      z_bucket_entry_t item;

      block = __sset_block_create(&(entry->key));
      Z_ASSERT(block != NULL, "TODO: Malloc failed, check me");
      z_dlink_move_tail(blkseq, &(block->blkseq));

      do {
        __sset_fill_bucket_entry(&item, &(entry->key), &(entry->value), &last_key);
        if (z_bucket_variable.append(block->data, &item))
          break;

        z_memcpy(last_key_buf + item.kprefix, item.key.data, item.key.size);
        z_byte_slice_set(&last_key, last_key_buf, item.kprefix + item.key.size);
      } while ((entry = z_map_merger_next(&merger)) != NULL);

      __sset_block_finalize(block, &last_key);
      z_byte_slice_clear(&last_key);
    }

    z_dlink_del_for_each_entry(node_blkseq, block, struct sset_block, blkseq, {
      __sset_block_free(block);
    });
  }
}

/*
 * A B C D [data] E F G
 */
static void __sset_node_sync (raleighsl_sset_t *sset,
                              z_dlink_node_t *blkseq,
                              struct sset_node *node)
{
  z_skip_list_iterator_t iter;
  z_dlink_node_t node_blkseq;
  struct sset_entry *entry;
  z_byte_slice_t empty;

  z_dlink_init(&node_blkseq);
  z_byte_slice_clear(&empty);

  // Generate the new blocks from the in-memory data
  z_iterator_open(&iter, &(node->skip_list));
  entry = __SSET_ENTRY(z_iterator_begin(&iter));
  while (entry != NULL) {
    struct sset_block *block;
    z_byte_slice_t *last_key;

    block = __sset_block_create(&(entry->key.slice));
    if (Z_MALLOC_IS_NULL(block))
      break;

    last_key = &empty;
    do {
      if (__sset_block_add_entry(block, entry, last_key))
        break;

      last_key = &(entry->key.slice);
      entry = __SSET_ENTRY(z_iterator_next(&iter));
    } while (entry != NULL);
    __sset_block_finalize(block, last_key);
    z_dlink_add_tail(&node_blkseq, &(block->blkseq));
  }
  z_iterator_close(&iter);

  if (node->block != NULL) {
    __sset_node_sync_merge(sset, node, &node_blkseq, blkseq);
    node->block = NULL;
  } else {
    struct sset_block *block;
    /* TODO: Dlink Concat? */
    z_dlink_for_each_safe_entry(&node_blkseq, block, struct sset_block, blkseq, {
      z_dlink_move_tail(blkseq, &(block->blkseq));
    });
  }
}

/* ============================================================================
 *  PRIVATE  SSet Txn Locks methods
 */
#define __sset_txn_get_func(func, sset, key)                                  \
  (Z_CAST(struct sset_txn, func(&((sset)->txn_locks),                         \
                                __sset_txn_key_compare, key)))

#define __sset_txn_lookup(sset, key)                                          \
  __sset_txn_get_func(z_skip_list_get_custom, sset, key)

static int __sset_txn_compare (void *udata, const void *a, const void *b) {
  struct sset_txn *txn_a = (struct sset_txn *)a;
  struct sset_txn *txn_b = (struct sset_txn *)b;
  return(z_bytes_ref_compare(&(txn_a->entry->key), &(txn_b->entry->key)));
}

static int __sset_txn_key_compare (void *udata, const void *a, const void *key) {
  struct sset_txn *txn = (struct sset_txn *)a;
  return(z_bytes_ref_compare(&(txn->entry->key), Z_CONST_BYTES_REF(key)));
}

static struct sset_txn *__sset_txn_alloc (uint64_t txn_id, enum sset_txn_type type, struct sset_entry *entry) {
  struct sset_txn *txn;

  txn = z_memory_struct_alloc(z_global_memory(), struct sset_txn);
  if (Z_MALLOC_IS_NULL(txn)) {
    return(NULL);
  }

  txn->entry = entry;
  txn->txn_id = txn_id;
  txn->type = type;
  return(txn);
}

static void __sset_txn_free (void *udata, void *obj) {
  struct sset_txn *txn = (struct sset_txn *)obj;
  Z_ASSERT(txn->entry == NULL, "SSET-TXN entry must be released by apply/revert");
  z_memory_struct_free(z_global_memory(), struct sset_txn, txn);
}

static struct sset_txn *__sset_txn_has_lock (raleighsl_sset_t *sset, const z_bytes_ref_t *key) {
  return((sset->txn_locks.size > 0) ? __sset_txn_lookup(sset, key) : NULL);
}

static raleighsl_errno_t __sset_txn_add (raleighsl_t *fs,
                                         raleighsl_object_t *object,
                                         raleighsl_transaction_t *transaction,
                                         enum sset_txn_type type,
                                         struct sset_entry *entry)
{
  raleighsl_sset_t *sset = RALEIGHSL_SSET(object->membufs);
  struct sset_txn *txn;

  txn = __sset_txn_alloc(raleighsl_txn_id(transaction), type, entry);
  if (Z_MALLOC_IS_NULL(txn)) {
    __sset_entry_free(sset, entry);
    return(RALEIGHSL_ERRNO_NO_MEMORY);
  }

  if (z_skip_list_put(&(sset->txn_locks), txn)) {
    __sset_txn_free(sset, txn);
    __sset_entry_free(sset, entry);
    return(RALEIGHSL_ERRNO_NO_MEMORY);
  }

  if (raleighsl_transaction_add(fs, transaction, object, txn)) {
    z_skip_list_rollback(&(sset->txn_locks));
    __sset_entry_free(sset, entry);
    return(RALEIGHSL_ERRNO_NO_MEMORY);
  }

  z_skip_list_commit(&(sset->txn_locks));
  return(RALEIGHSL_ERRNO_NONE);
}

static void __sset_txn_remove (raleighsl_sset_t *sset,
                               struct sset_txn *txn,
                               int release_entity)
{
  __sset_txn_get_func(z_skip_list_remove_custom, sset, &(txn->entry->key));
  if (release_entity)
    __sset_entry_free(sset, txn->entry);
  txn->entry = NULL;
  z_skip_list_commit(&(sset->txn_locks));
}

/* ============================================================================
 *  PRIVATE  SSet Txn Apply methods
 */
static raleighsl_errno_t __sset_apply_insert (raleighsl_sset_t *sset,
                                              struct sset_node *node,
                                              struct sset_entry *entry)
{
  /* Add directly to the table */
  if (z_skip_list_put(&(node->skip_list), entry)) {
    //__sset_entry_free(sset, entry);
    return(RALEIGHSL_ERRNO_NO_MEMORY);
  }
  node->bufsize += (entry->key.slice.size + entry->value.slice.size);
  z_dlink_move(&(sset->dirtyq), &(node->dirtyq));
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __sset_apply_remove (raleighsl_sset_t *sset,
                                              struct sset_node *node,
                                              const z_bytes_ref_t *key,
                                              z_bytes_ref_t *value)
{
  struct sset_entry *entry;
  z_byte_slice_t svalue;

  if ((entry = __SSET_ENTRY(z_skip_list_remove_custom(&(node->skip_list),
                            __sset_entry_key_compare, key))) != NULL)
  {
    node->bufsize -= (entry->key.slice.size + entry->value.slice.size);
    z_bytes_ref_acquire(value, &(entry->value));
  } else if (node->block != NULL &&
             !z_bucket_search(&z_bucket_variable, node->block->data, &(key->slice), &svalue))
  {
    z_atomic_inc(&(node->block->refs));
    z_bytes_ref_set(value, &(svalue), &__sset_block_vtable_refs, node->block);
  } else {
    return(RALEIGHSL_ERRNO_DATA_KEY_NOT_FOUND);
  }

  z_dlink_move(&(sset->dirtyq), &(node->dirtyq));
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __sset_apply_update (raleighsl_sset_t *sset,
                                              struct sset_node *node,
                                              struct sset_entry *entry)
{
  raleighsl_errno_t errno;

  /* Remove the old value */
  if ((errno = __sset_apply_remove(sset, node, &(entry->key), NULL)))
    return(errno);

  /* Add directly to the table */
  return(__sset_apply_insert(sset, node, entry));
}

/* ============================================================================
 *  PUBLIC SSet WRITE methods
 */
raleighsl_errno_t raleighsl_sset_insert (raleighsl_t *fs,
                                         raleighsl_transaction_t *transaction,
                                         raleighsl_object_t *object,
                                         int allow_update,
                                         const z_bytes_ref_t *key,
                                         const z_bytes_ref_t *value)
{
  raleighsl_sset_t *sset = RALEIGHSL_SSET(object->membufs);
  struct sset_entry *entry;
  struct sset_node *node;
  struct sset_txn *txn;

  /* Verify that no other transaction is holding the key-lock */
  txn = __sset_txn_has_lock(sset, key);
  if (txn != NULL && (transaction != NULL && raleighsl_txn_id(transaction) != txn->txn_id))
    return(RALEIGHSL_ERRNO_TXN_LOCKED_KEY);

  /* Update Transaction Value */
  if (txn != NULL) {
    Z_ASSERT(txn->txn_id == raleighsl_txn_id(transaction), "TXN-ID Mismatch");
    switch (txn->type) {
      case SSET_TXN_INSERT:
        __sset_entry_set_value(sset, txn->entry, value);
        break;
      case SSET_TXN_UPDATE:
        return(RALEIGHSL_ERRNO_DATA_KEY_EXISTS);
        break;
      case SSET_TXN_REMOVE:
        __sset_entry_set_value(sset, txn->entry, value);
        txn->type = SSET_TXN_UPDATE;
        break;
    }
    return(RALEIGHSL_ERRNO_NONE);
  }

  /* Lookup Node */
  node = __sset_node_lookup(sset, key);
  Z_ASSERT(node != NULL, "Unable to find a node");

  if (!allow_update && __sset_node_search(node, key, NULL))
    return(RALEIGHSL_ERRNO_DATA_KEY_EXISTS);

  /* Allocate the new entry */
  entry = __sset_entry_alloc(sset, key, value, node);
  if (Z_MALLOC_IS_NULL(entry)) {
    return(RALEIGHSL_ERRNO_NO_MEMORY);
  }

  /* Add to the transaction */
  if (transaction != NULL) {
    return(__sset_txn_add(fs, object, transaction, SSET_TXN_INSERT, entry));
  }

  /* Add directly to the table */
  return(__sset_apply_insert(sset, node, entry));
}

raleighsl_errno_t raleighsl_sset_update (raleighsl_t *fs,
                                         raleighsl_transaction_t *transaction,
                                         raleighsl_object_t *object,
                                         const z_bytes_ref_t *key,
                                         const z_bytes_ref_t *value,
                                         z_bytes_ref_t *old_value)
{
  raleighsl_errno_t errno;

  if ((errno = raleighsl_sset_remove(fs, transaction, object, key, old_value)))
    return(errno);

  if ((errno = raleighsl_sset_insert(fs, transaction, object, 1, key, value)))
    return(errno);

  return(RALEIGHSL_ERRNO_NONE);
}

raleighsl_errno_t raleighsl_sset_remove (raleighsl_t *fs,
                                         raleighsl_transaction_t *transaction,
                                         raleighsl_object_t *object,
                                         const z_bytes_ref_t *key,
                                         z_bytes_ref_t *value)
{
  raleighsl_sset_t *sset = RALEIGHSL_SSET(object->membufs);
  struct sset_node *node;
  struct sset_txn *txn;

  /* Verify that no other transaction is holding the key-lock */
  txn = __sset_txn_has_lock(sset, key);
  if (txn != NULL && (transaction != NULL && raleighsl_txn_id(transaction) != txn->txn_id))
    return(RALEIGHSL_ERRNO_TXN_LOCKED_KEY);

  if (txn != NULL) {
    Z_ASSERT(txn->txn_id == raleighsl_txn_id(transaction), "TXN-ID Mismatch");
    switch (txn->type) {
      case SSET_TXN_INSERT:
        __sset_txn_remove(sset, txn, 1);
        break;
      case SSET_TXN_UPDATE:
      case SSET_TXN_REMOVE:
        txn->type = SSET_TXN_REMOVE;
        break;
    }
    return(RALEIGHSL_ERRNO_NONE);
  }

  /* Lookup Node */
  node = __sset_node_lookup(sset, key);
  Z_ASSERT(node != NULL, "Unable to find a node");
#if 0
  /* Add to the transaction */
  if (transaction != NULL) {
    if (!__sset_node_search(node, key, value))
      return(RALEIGHSL_ERRNO_DATA_KEY_NOT_FOUND);
    return(__sset_txn_add(fs, object, transaction, SSET_TXN_REMOVE, entry));
  }
#endif
  /* Apply directly to the table */
  return(__sset_apply_remove(sset, node, key, value));
}

/* ============================================================================
 *  PUBLIC SSet READ methods
 */
raleighsl_errno_t raleighsl_sset_get (raleighsl_t *fs,
                                      const raleighsl_transaction_t *transaction,
                                      raleighsl_object_t *object,
                                      const z_bytes_ref_t *key,
                                      z_bytes_ref_t *value)
{
  raleighsl_sset_t *sset = RALEIGHSL_SSET(object->membufs);
  struct sset_node *node;

  if (transaction != NULL) {
    struct sset_txn *txn;
    txn = __sset_txn_has_lock(sset, key);
    if (txn != NULL && txn->txn_id == raleighsl_txn_id(transaction)) {
      if (txn->type == SSET_TXN_REMOVE)
        return(RALEIGHSL_ERRNO_DATA_KEY_NOT_FOUND);

      z_bytes_ref_acquire(value, &(txn->entry->value));
      return(RALEIGHSL_ERRNO_NONE);
    }
  }

  /* Lookup Node */
  node = __sset_node_lookup(sset, key);
  Z_ASSERT(node != NULL, "Unable to find a node");

  if (!__sset_node_search(node, key, value))
    return(RALEIGHSL_ERRNO_DATA_KEY_NOT_FOUND);
  return(RALEIGHSL_ERRNO_NONE);
}

raleighsl_errno_t raleighsl_sset_scan (raleighsl_t *fs,
                                       const raleighsl_transaction_t *transaction,
                                       raleighsl_object_t *object,
                                       const z_bytes_ref_t *key,
                                       int include_key,
                                       size_t count,
                                       z_array_t *keys,
                                       z_array_t *values)
{
  raleighsl_sset_t *sset = RALEIGHSL_SSET(object->membufs);
  z_skip_list_iterator_t iter_node;
  const z_byte_slice_t *key_slice;
  struct sset_node *node;

Z_TRACE_TIME(sset_scan, {
  z_iterator_open(&iter_node, &(sset->skip_list));
  if (key == NULL) {
    key_slice = NULL;
    node = __SSET_NODE(z_iterator_begin(&iter_node));
  } else {
    key_slice = z_bytes_slice(key);
    node = __SSET_NODE(z_iterator_seek_le(&iter_node, __sset_node_compare, key));
  }

  while (node != NULL && count > 0) {
    struct sset_entry_map_iterator iter_mem;
    z_bucket_iterator_t iter_block;
    const z_map_entry_t *entry;
    z_map_merger_t merger;

    z_map_merger_open(&merger);

    if (node->block != NULL) {
      z_bucket_iterator_open(&iter_block, &z_bucket_variable, node->block->data,
                             &__sset_block_vtable_refs, node->block);
      z_map_iterator_seek_to(Z_MAP_ITERATOR(&iter_block), key_slice, include_key);
      z_map_merger_add(&merger, Z_MAP_ITERATOR(&iter_block));
    }

    if (node->skip_list.size > 0) {
      __sset_entry_map_open(&iter_mem, &(node->skip_list));
      z_map_iterator_seek_to(Z_MAP_ITERATOR(&iter_mem), key_slice, include_key);
      z_map_merger_add(&merger, Z_MAP_ITERATOR(&iter_mem));
    }

    while (count > 0 && (entry = z_map_merger_next(&merger)) != NULL) {
      z_bytes_ref_t *value_ref;
      z_bytes_ref_t *key_ref;

      if (transaction != NULL) {
        struct sset_txn *txn = __sset_txn_has_lock(sset, key);
        if (txn != NULL && raleighsl_txn_id(transaction) == txn->txn_id) {
          if (txn->type == SSET_TXN_REMOVE)
            continue;

          key_ref = z_array_push_back(keys);
          value_ref = z_array_push_back(values);
          z_bytes_ref_acquire(key_ref, &(txn->entry->key));
          z_bytes_ref_acquire(value_ref, &(txn->entry->value));
          count--;
          continue;
        }
      }

      key_ref = z_array_push_back(keys);
      value_ref = z_array_push_back(values);
      z_map_iterator_get_refs(merger.smallest_iter, key_ref, value_ref);
      count--;
    }
    /* TODO: Close iter_block and iter_mem */

    key_slice = NULL;
    node = __SSET_NODE(z_iterator_next(&iter_node));
  }
  z_iterator_close(&iter_node);
});
  return(RALEIGHSL_ERRNO_NONE);
}

/* ============================================================================
 *  SSet Object Plugin
 */
static raleighsl_errno_t __object_create (raleighsl_t *fs,
                                          raleighsl_object_t *object)
{
  raleighsl_sset_t *sset;

  sset = z_memory_struct_alloc(z_global_memory(), raleighsl_sset_t);
  if (Z_MALLOC_IS_NULL(sset))
    return(RALEIGHSL_ERRNO_NO_MEMORY);

  if (z_skip_list_alloc(&(sset->skip_list),
                        __sset_node_compare, __sset_node_free,
                        sset, (size_t)object) == NULL)
  {
    z_memory_struct_free(z_global_memory(), raleighsl_sset_t, sset);
    return(RALEIGHSL_ERRNO_NO_MEMORY);
  }

  /* TODO */
  z_skip_list_put_direct(&(sset->skip_list), __sset_node_alloc(sset, NULL));

  if (z_skip_list_alloc(&(sset->txn_locks),
                        __sset_txn_compare, __sset_txn_free,
                        sset, (size_t)object) == NULL)
  {
    z_skip_list_free(&(sset->skip_list));
    z_memory_struct_free(z_global_memory(), raleighsl_sset_t, sset);
    return(RALEIGHSL_ERRNO_NO_MEMORY);
  }

  z_dlink_init(&(sset->dirtyq));

  object->membufs = sset;
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __object_close (raleighsl_t *fs,
                                         raleighsl_object_t *object)
{
  raleighsl_sset_t *sset = RALEIGHSL_SSET(object->membufs);
  z_skip_list_free(&(sset->txn_locks));
  z_skip_list_free(&(sset->skip_list));
  z_memory_struct_free(z_global_memory(), raleighsl_sset_t, sset);
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __object_apply (raleighsl_t *fs,
                                         raleighsl_object_t *object,
                                         void *mutation)
{
  raleighsl_sset_t *sset = RALEIGHSL_SSET(object->membufs);
  struct sset_txn *txn = (struct sset_txn *)mutation;
  raleighsl_errno_t errno;

  switch (txn->type) {
    case SSET_TXN_INSERT:
      errno = __sset_apply_insert(sset, txn->entry->node, txn->entry);
      break;
    case SSET_TXN_UPDATE:
      errno = __sset_apply_update(sset, txn->entry->node, txn->entry);
      break;
    case SSET_TXN_REMOVE:
      errno = __sset_apply_remove(sset, txn->entry->node, &(txn->entry->key), NULL);
      break;
    default:
      errno = RALEIGHSL_ERRNO_NOT_IMPLEMENTED;
      Z_LOG_WARN("Unknown sset_txn->type=%d", txn->type);
      break;
  }

  __sset_txn_remove(sset, txn, 0);
  return(errno);
}

static raleighsl_errno_t __object_revert (raleighsl_t *fs,
                                          raleighsl_object_t *object,
                                          void *mutation)
{
  raleighsl_sset_t *sset = RALEIGHSL_SSET(object->membufs);
  struct sset_txn *txn = (struct sset_txn *)mutation;
  __sset_txn_remove(sset, txn, 1);
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __object_commit (raleighsl_t *fs,
                                          raleighsl_object_t *object)
{
  raleighsl_sset_t *sset = RALEIGHSL_SSET(object->membufs);
  struct sset_node *node;
  int do_flush = 0;

  z_dlink_for_each_entry(&(sset->dirtyq), node, struct sset_node, dirtyq, {
    z_skip_list_commit(&(node->skip_list));
    do_flush |= (node->bufsize >= __SSET_SYNC_SIZE);
  });

  if (do_flush)
    object->plug->sync(fs, object);
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __object_rollback (raleighsl_t *fs,
                                            raleighsl_object_t *object)
{
  raleighsl_sset_t *sset = RALEIGHSL_SSET(object->membufs);
  z_skip_list_rollback(&(sset->skip_list));
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __object_sync (raleighsl_t *fs,
                                        raleighsl_object_t *object)
{
  raleighsl_sset_t *sset = RALEIGHSL_SSET(object->membufs);
  struct sset_node *node;
  int has_empty_node = 1;
  z_dlink_node_t blkseq;

  z_dlink_init(&blkseq);

Z_TRACE_TIME(sset_sync, {
  Z_LOG_TRACE("==============================================================");
  Z_LOG_TRACE("BLOCKS %u", sset->skip_list.size);
  z_dlink_del_for_each_entry(&(sset->dirtyq), node, struct sset_node, dirtyq, {
    if (node->bufsize >= __SSET_SYNC_SIZE) {
      __sset_node_sync(sset, &blkseq, node);
      has_empty_node &= z_byte_slice_is_not_empty(&(node->key.slice));
      z_skip_list_remove_direct(&(sset->skip_list), __sset_node_compare, node);
    }
  });

  /* TODO: squeeze */

  /* Push back blocks on the list */
  Z_LOG_TRACE("Replace blocks");
  {
    /* Add the Replacement nodes */
    struct sset_block *block;
    z_dlink_del_for_each_entry(&blkseq, block, struct sset_block, blkseq, {
      struct sset_node *node;

      node = __sset_node_alloc(sset, block);
      __sset_block_debug(block);
      z_skip_list_put_direct(&(sset->skip_list), node);
    });

    /* Add an empty node */
    if (!has_empty_node) {
      node = __sset_node_alloc(sset, NULL);
      z_skip_list_put_direct(&(sset->skip_list), node);
    }
  }
  Z_LOG_TRACE("==============================================================");
});

  return(RALEIGHSL_ERRNO_NONE);
}

const raleighsl_object_plug_t raleighsl_object_sset = {
  .info = {
    .type = RALEIGHSL_PLUG_TYPE_OBJECT,
    .description = "Sorted-Set Object",
    .label       = "sset",
  },

  .create   = __object_create,
  .open     = NULL,
  .close    = __object_close,
  .commit   = __object_commit,
  .rollback = __object_rollback,
  .sync     = __object_sync,
  .unlink   = NULL,
  .apply    = __object_apply,
  .revert   = __object_revert,
};
