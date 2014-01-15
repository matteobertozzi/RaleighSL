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
#include <zcl/bucket.h>
#include <zcl/debug.h>
#include <zcl/dlink.h>
#include <zcl/bytes.h>
#include <zcl/time.h>

#include "sset.h"

#define RALEIGHSL_SSET(x)                 Z_CAST(raleighsl_sset_t, x)
#define __CONST_SSET_BLOCK(x)             Z_CONST_CAST(struct sset_block, x)
#define __CONST_SSET_ENTRY(x)             Z_CONST_CAST(struct sset_entry, x)
#define __CONST_SSET_ITEM(x)              Z_CONST_CAST(struct sset_item, x)
#define __CONST_SSET_TXN(x)               Z_CONST_CAST(struct sset_txn, x)
#define __SSET_BLOCK(x)                   Z_CAST(struct sset_block, x)
#define __SSET_ENTRY(x)                   Z_CAST(struct sset_entry, x)
#define __SSET_ITEM(x)                    Z_CAST(struct sset_item, x)
#define __SSET_TXN(x)                     Z_CAST(struct sset_txn, x)

struct sset_item {
  z_tree_node_t __node__;

  z_bytes_ref_t key;
  z_bytes_ref_t value;
};

enum sset_txn_type {
  SSET_TXN_INSERT,
  SSET_TXN_UPDATE,
  SSET_TXN_REMOVE
};

struct sset_txn {
  raleighsl_txn_atom_t __txn_atom__;
  z_tree_node_t __node__;

  uint64_t txn_id;
  z_dlink_node_t commitq;

  struct sset_node *node;
  struct sset_item *item;
  struct sset_txn *parent;

  enum sset_txn_type type;
};

/* TODO: REPLACE: Test Values */
#define __SSET_BLOCK_SIZE         (8 << 10)
#define __SSET_SYNC_SIZE          (4 << 10)
#define __SSET_BLOCK_MERGE_SIZE   (__SSET_BLOCK_SIZE - (__SSET_BLOCK_SIZE >> 2))

struct sset_block {
  z_dlink_node_t blkseq;

  uint32_t refs;
  uint8_t data[__SSET_BLOCK_SIZE];
};

struct sset_node {
  z_tree_node_t __node__;

  z_dlink_node_t dirtyq;
  z_dlink_node_t commitq;

  z_tree_node_t *mem_data;
  z_tree_node_t *txn_locks;
  struct sset_block *block;

  /* Node Stats */
  unsigned int bufsize;
  unsigned int min_ksize;
  unsigned int max_ksize;
  unsigned int min_vsize;
  unsigned int max_vsize;
};

struct sset_entry {
  struct sset_txn *txn;
  struct sset_item *item;
  struct sset_block *block;
  z_bytes_ref_t blk_value;
  z_bytes_ref_t *value;
};

typedef struct raleighsl_sset {
  z_tree_node_t *root;            /* sset-node */
  z_dlink_node_t txnq;            /* Pending txn-add   (txn->commitq) */
  z_dlink_node_t dirtyq;          /* Pending txn-apply (node->dirtyq) */
  z_dlink_node_t rm_nodes;
  z_dlink_node_t add_nodes;
} raleighsl_sset_t;

struct sset_txn_iter {
  __Z_MAP_ITERABLE__
  z_map_entry_t entry;
  z_tree_iter_t iter;
  uint64_t txn_id;
};

struct sset_mem_iter {
  __Z_MAP_ITERABLE__
  z_map_entry_t entry;
  z_tree_iter_t iter;
};

/*
 * [WRITE] -> __sset_txn_add() -> [COMMIT]
 * [TXN-WRITE] -> __sset_txn_add() -> [COMMIT] -> ... -> [TXN-APPLY] -> [COMMIT]
 */

/* ============================================================================
 *  PRIVATE SSet Item methods
 */
#define __sset_item_size(item)                                                \
  ((item)->key.slice.size + (item)->value.slice.size)

#define __sset_item_is_delete_marker(item)                                    \
  ((item)->__node__.udata)

#define __sset_item_attach(tree_root, item)                                   \
  z_tree_node_attach(&__sset_item_tree_info, tree_root,                       \
                     &((item)->__node__), NULL)

#define __sset_item_detach(tree_root, key)                                    \
  z_tree_node_detach(&__sset_item_tree_info, tree_root, key, NULL)

#define __sset_item_node_get(item_tree, key)                                  \
  z_tree_node_lookup(item_tree, __sset_item_node_key_compare, key, NULL)

static int __sset_item_node_compare (void *udata, const void *a, const void *b) {
  const struct sset_item *ea = z_container_of(a, const struct sset_item, __node__);
  const struct sset_item *eb = z_container_of(b, const struct sset_item, __node__);
  return((ea == eb) ? 0 : z_bytes_ref_compare(&(ea->key), &(eb->key)));
}

static int __sset_item_node_key_compare (void *udata, const void *a, const void *key) {
  const struct sset_item *ea = z_container_of(a, const struct sset_item, __node__);
  Z_ASSERT(a == &(ea->__node__), "container resolve failed");
  return(z_bytes_ref_compare(&(ea->key), Z_CONST_BYTES_REF(key)));
}

static void __sset_item_free (struct sset_item *item) {
  z_bytes_ref_release(&(item->key));
  z_bytes_ref_release(&(item->value));
  z_memory_struct_free(z_global_memory(), struct sset_item, item);
}

static void __sset_item_node_free (void *udata, void *obj) {
  struct sset_item *item = z_container_of(obj, struct sset_item, __node__);
  Z_ASSERT(obj == &(item->__node__), "container resolve failed");
  __sset_item_free(item);
}

static struct sset_item *__sset_item_alloc (const z_bytes_ref_t *key,
                                            const z_bytes_ref_t *value,
                                            int is_delete_marker)
{
  struct sset_item *item;

  item = z_memory_struct_alloc(z_global_memory(), struct sset_item);
  if (Z_MALLOC_IS_NULL(item))
    return(NULL);

  item->__node__.udata = !!is_delete_marker;

  Z_ASSERT(key != NULL, "Must have a key");
  z_bytes_ref_acquire(&(item->key), key);
  if (value != NULL) {
    z_bytes_ref_acquire(&(item->value), value);
  } else {
    z_bytes_ref_reset(&(item->value));
  }
  return(item);
}

static const z_tree_info_t __sset_item_tree_info = {
  .plug         = &z_tree_avl,
  .node_compare = __sset_item_node_compare,
  .key_compare  = __sset_item_node_key_compare,
  .node_free    = __sset_item_node_free,
};

/* ============================================================================
 *  PRIVATE SSet Items Mem-Map methods
 */
static const z_vtable_map_iterator_t __sset_mem_iterator;

static int __sset_item_node_to_map (struct sset_mem_iter *self,
                                    const z_tree_node_t *node,
                                    z_map_entry_t *map)
{
  if (Z_LIKELY(node != NULL)) {
    struct sset_item *item = z_container_of(node, struct sset_item, __node__);
    z_byte_slice_copy(&(map->key), &(item->key.slice));
    z_byte_slice_copy(&(map->value), &(item->value.slice));
    map->is_delete_marker = __sset_item_is_delete_marker(item);
    Z_MAP_ITERATOR_HEAD(self).object = item;
    return(1);
  }

  Z_MAP_ITERATOR_HEAD(self).object = NULL;
  return(0);
}

static int __sset_mem_iter_open (void *self, z_tree_node_t *root) {
  struct sset_mem_iter *iter = (struct sset_mem_iter *)self;
  Z_MAP_ITERATOR_INIT(self, &__sset_mem_iterator, &z_vtable_bytes_refs, NULL);
  z_tree_iter_open(&(iter->iter), root);
  return(0);
}

static int __sset_mem_iter_begin (void *self) {
  struct sset_mem_iter *iter = (struct sset_mem_iter *)self;
  const z_tree_node_t *node = z_tree_iter_begin(&(iter->iter));
  return(__sset_item_node_to_map(self, node, &(iter->entry)));
}

static int __sset_mem_iter_next (void *self) {
  struct sset_mem_iter *iter = (struct sset_mem_iter *)self;
  const z_tree_node_t *node = z_tree_iter_next(&(iter->iter));
  return(__sset_item_node_to_map(self, node, &(iter->entry)));
}

static int __sset_mem_iter_seek (void *self, const z_byte_slice_t *key) {
  struct sset_mem_iter *iter = (struct sset_mem_iter *)self;
  const z_tree_node_t *node = z_tree_iter_seek_le(&(iter->iter),
                                                  __sset_item_node_key_compare,
                                                  key, NULL);
  __sset_item_node_to_map(self, node, &(iter->entry));
  return(0);
}

const z_map_entry_t *__sset_mem_iter_current (const void *self) {
  const struct sset_mem_iter *iter = (const struct sset_mem_iter *)self;
  void *item = Z_MAP_ITERATOR_HEAD(iter).object;
  return((item != NULL) ? &(iter->entry) : NULL);
}

static void __sset_mem_iter_get_refs (const void *self, z_bytes_ref_t *key, z_bytes_ref_t *value) {
  const struct sset_mem_iter *iter = (const struct sset_mem_iter *)self;
  struct sset_item *item = __SSET_ITEM(Z_MAP_ITERATOR_HEAD(iter).object);
  if (Z_LIKELY(item != NULL)) {
    z_bytes_ref_acquire(key, &(item->key));
    z_bytes_ref_acquire(value, &(item->value));
  }
}

static const z_vtable_map_iterator_t __sset_mem_iterator = {
  .begin    = __sset_mem_iter_begin,
  .next     = __sset_mem_iter_next,
  .seek     = __sset_mem_iter_seek,
  .current  = __sset_mem_iter_current,
  .get_refs = __sset_mem_iter_get_refs,
};

/* ============================================================================
 *  PRIVATE SSet TXN methods
 */
#define __sset_txn_id(user_txn)                                               \
  ((user_txn != NULL) ? raleighsl_txn_id(user_txn) : 0)

#define __sset_txn_attach(txn)                                                \
  z_tree_node_attach(&__sset_txn_tree_info, &((txn)->node->txn_locks),        \
                     &((txn)->__node__), NULL)

#define __sset_txn_detach(txn)                                                \
  z_tree_node_detach(&__sset_txn_tree_info, &((txn)->node->txn_locks),        \
                     &((txn)->__node__), NULL)

#define __sset_txn_node_get(txn_tree, key)                                    \
  z_tree_node_lookup(txn_tree, __sset_txn_node_key_compare, key, NULL)

#define __sset_txn_key_locked(entry_txn, user_txn)                            \
  ((entry_txn) != NULL && (entry_txn)->txn_id != __sset_txn_id(user_txn))

#define __sset_txn_is_current(entry_txn, user_txn)                            \
  ((entry_txn) != NULL && (entry_txn)->txn_id == __sset_txn_id(user_txn))

static struct sset_node *__sset_node_from_tree (const z_tree_node_t *tree_node) {
  return(tree_node ? z_container_of(tree_node, struct sset_node, __node__) : NULL);
}

static int __sset_txn_node_compare (void *udata, const void *a, const void *b) {
  const struct sset_txn *ea = z_container_of(a, const struct sset_txn, __node__);
  const struct sset_txn *eb = z_container_of(b, const struct sset_txn, __node__);
  return((ea == eb) ? 0 : z_bytes_ref_compare(&(ea->item->key), &(eb->item->key)));
}

static int __sset_txn_node_key_compare (void *udata, const void *a, const void *key) {
  const struct sset_txn *ea = z_container_of(a, const struct sset_txn, __node__);
  return(z_bytes_ref_compare(&(ea->item->key), Z_CONST_BYTES_REF(key)));
}

static void __sset_txn_free (struct sset_txn *txn) {
  z_memory_struct_free(z_global_memory(), struct sset_txn, txn);
}

static void __sset_txn_node_free (void *udata, void *obj) {
  struct sset_txn *txn = z_container_of(obj, struct sset_txn, __node__);
  __sset_txn_free(txn);
}

static struct sset_txn *__sset_txn_alloc (uint64_t txn_id,
                                          struct sset_node *node,
                                          enum sset_txn_type type,
                                          struct sset_item *item)
{
  struct sset_txn *txn;

  txn = z_memory_struct_alloc(z_global_memory(), struct sset_txn);
  if (Z_MALLOC_IS_NULL(txn))
    return(NULL);

  txn->txn_id = txn_id;
  z_dlink_init(&(txn->commitq));

  txn->node = node;
  txn->item = item;
  txn->parent = NULL;
  txn->type = type;
  return(txn);
}

static const z_tree_info_t __sset_txn_tree_info = {
  .plug         = &z_tree_avl,
  .node_compare = __sset_txn_node_compare,
  .key_compare  = __sset_txn_node_compare,
  .node_free    = __sset_txn_node_free,
};

/* ============================================================================
 *  PRIVATE SSet Items TXN-Map methods
 */
static const z_vtable_map_iterator_t __sset_txn_iterator;

static int __sset_txn_node_to_map (struct sset_txn_iter *self,
                                   const z_tree_node_t *node,
                                   z_map_entry_t *map)
{
  if (Z_LIKELY(node != NULL)) {
    struct sset_txn *txn = z_container_of(node, struct sset_txn, __node__);
    z_byte_slice_copy(&(map->key), &(txn->item->key.slice));
    z_byte_slice_copy(&(map->value), &(txn->item->value.slice));
    map->is_delete_marker = txn->type == SSET_TXN_REMOVE;
    Z_MAP_ITERATOR_HEAD(self).object = txn;
    return(1);
  }

  Z_MAP_ITERATOR_HEAD(self).object = NULL;
  return(0);
}

static const z_tree_node_t *__sset_txn_iter_skip_txns (struct sset_txn_iter *iter,
                                                       const z_tree_node_t *node)
{
  while (node != NULL) {
    struct sset_txn *txn = z_container_of(node, struct sset_txn, __node__);

    /* TODO: Add support for "read-uncommitted" */
    if (txn->txn_id == iter->txn_id)
      return(node);

    node = z_tree_iter_next(&(iter->iter));
  }
  return(node);
}

static int __sset_txn_iter_open (void *self, z_tree_node_t *root, uint64_t txn_id) {
  struct sset_txn_iter *iter = (struct sset_txn_iter *)self;
  Z_MAP_ITERATOR_INIT(self, &__sset_txn_iterator, &z_vtable_bytes_refs, NULL);
  iter->txn_id = txn_id;
  z_tree_iter_open(&(iter->iter), root);
  return(0);
}

static int __sset_txn_iter_begin (void *self) {
  struct sset_txn_iter *iter = (struct sset_txn_iter *)self;
  const z_tree_node_t *node = z_tree_iter_begin(&(iter->iter));
  node = __sset_txn_iter_skip_txns(iter, node);
  return(__sset_txn_node_to_map(self, node, &(iter->entry)));
}

static int __sset_txn_iter_next (void *self) {
  struct sset_txn_iter *iter = (struct sset_txn_iter *)self;
  const z_tree_node_t *node = z_tree_iter_next(&(iter->iter));
  node = __sset_txn_iter_skip_txns(iter, node);
  return(__sset_txn_node_to_map(self, node, &(iter->entry)));
}

static int __sset_txn_iter_seek (void *self, const z_byte_slice_t *key) {
  struct sset_txn_iter *iter = (struct sset_txn_iter *)self;
  const z_tree_node_t *node = z_tree_iter_seek_le(&(iter->iter),
                                                  __sset_item_node_key_compare,
                                                  key, NULL);
  node = __sset_txn_iter_skip_txns(iter, node);
  __sset_txn_node_to_map(self, node, &(iter->entry));
  return(0);
}

const z_map_entry_t *__sset_txn_iter_current (const void *self) {
  const struct sset_txn_iter *iter = (const struct sset_txn_iter *)self;
  void *item = Z_MAP_ITERATOR_HEAD(iter).object;
  return((item != NULL) ? &(iter->entry) : NULL);
}

static void __sset_txn_iter_get_refs (const void *self, z_bytes_ref_t *key, z_bytes_ref_t *value) {
  const struct sset_txn_iter *iter = (const struct sset_txn_iter *)self;
  struct sset_txn *txn = __SSET_TXN(Z_MAP_ITERATOR_HEAD(iter).object);
  if (Z_LIKELY(txn != NULL)) {
    z_bytes_ref_acquire(key, &(txn->item->key));
    z_bytes_ref_acquire(value, &(txn->item->value));
  }
}

static const z_vtable_map_iterator_t __sset_txn_iterator = {
  .begin    = __sset_txn_iter_begin,
  .next     = __sset_txn_iter_next,
  .seek     = __sset_txn_iter_seek,
  .current  = __sset_txn_iter_current,
  .get_refs = __sset_txn_iter_get_refs,
};

/* ============================================================================
 *  PRIVATE SSet Block methods
 */
#define __sset_block_type(block)                                    \
  (&z_bucket_vprefix)

#define __sset_block_first_key(block, key)                          \
  do {                                                              \
    if ((block) != NULL) {                                          \
      __sset_block_type(block)->first_key((block)->data, key);      \
    } else {                                                        \
      z_byte_slice_clear(key);                                      \
    }                                                               \
  } while (0)

static struct sset_block *__sset_block_alloc (void) {
  struct sset_block *block;

  block = z_memory_struct_alloc(z_global_memory(), struct sset_block);
  if (Z_MALLOC_IS_NULL(block))
    return(NULL);

  z_dlink_init(&(block->blkseq));
  block->refs = 1;
  return(block);
}

static void __sset_block_free (struct sset_block *self) {
  if (self == NULL || z_atomic_dec(&(self->refs)) > 0)
    return;
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

static struct sset_block *__sset_block_create (void) {
  struct sset_block *block;

  block = __sset_block_alloc();
  if (Z_MALLOC_IS_NULL(block))
    return(NULL);

  __sset_block_type(block)->create(block->data, __SSET_BLOCK_SIZE);
  return(block);
}

static void __sset_block_finalize (struct sset_block *self) {
  __sset_block_type(self)->finalize(self->data);
}

static int __sset_block_add (struct sset_block *self,
                             size_t kprefix,
                             const z_byte_slice_t *key,
                             const z_byte_slice_t *value)
{
  z_bucket_entry_t item;
  item.kprefix = kprefix;
  z_byte_slice_copy(&(item.key), key);
  z_byte_slice_copy(&(item.value), value);
  return(__sset_block_type(self)->append(self->data, &item));
}

static int __sset_block_lookup (struct sset_block *self,
                                const z_bytes_ref_t *key,
                                z_bytes_ref_t *value)
{
  z_byte_slice_t val;
  int cmp = z_bucket_search(__sset_block_type(self), self->data, z_bytes_ref_slice(key), &val);
  if (!cmp && value != NULL) {
    z_bytes_ref_set(value, &val, &__sset_block_vtable_refs, self);
  }
  return(cmp);
}

/* ============================================================================
 *  PRIVATE SSet Node methods
 */
#define __sset_node_attach(sset, node)                            \
  z_tree_node_attach(&__sset_node_tree_info, &((sset)->root),     \
                     &((node)->__node__), NULL)

#define __sset_node_detach(sset, node)                            \
  z_tree_node_detach(&__sset_node_tree_info, &((sset)->root),     \
                     &((node)->__node__), NULL)

#define __sset_node_lookup(sset, key)                             \
  z_container_of(z_tree_node_floor((sset)->root,                  \
                                   __sset_node_key_compare,       \
                                   key, NULL),                    \
                 struct sset_node, __node__)

static int __sset_node_compare (void *udata, const void *a, const void *b) {
  const struct sset_node *ea = z_container_of(a, const struct sset_node, __node__);
  const struct sset_node *eb = z_container_of(b, const struct sset_node, __node__);
  Z_ASSERT(a == &(ea->__node__), "container resolve failed");
  Z_ASSERT(b == &(eb->__node__), "container resolve failed");
  if (ea != eb) {
    z_byte_slice_t ea_key;
    z_byte_slice_t eb_key;
    __sset_block_first_key(ea->block, &ea_key);
    __sset_block_first_key(eb->block, &eb_key);
    return(z_byte_slice_compare(&ea_key, &eb_key));
  }
  return(0);
}

static int __sset_node_key_compare (void *udata, const void *a, const void *key) {
  const struct sset_node *ea = z_container_of(a, const struct sset_node, __node__);
  z_byte_slice_t ea_key;
  __sset_block_first_key(ea->block, &ea_key);
  return(z_byte_slice_compare(&ea_key, z_bytes_ref_slice(Z_CONST_BYTES_REF(key))));
}

static void __sset_node_free (void *udata, void *obj) {
  struct sset_node *node = z_container_of(obj, struct sset_node, __node__);
  Z_ASSERT(obj == &(node->__node__), "container resolve failed");

  z_tree_node_clear(&__sset_item_tree_info, node->mem_data, udata);
  z_tree_node_clear(&__sset_txn_tree_info, node->txn_locks, udata);
  __sset_block_free(node->block);
  z_memory_struct_free(z_global_memory(), struct sset_node, node);
}

static const z_tree_info_t __sset_node_tree_info = {
  .plug         = &z_tree_avl,
  .node_compare = __sset_node_compare,
  .key_compare  = __sset_node_compare,
  .node_free    = __sset_node_free,
};

static struct sset_node *__sset_node_alloc (struct sset_block *block) {
  struct sset_node *node;

  node = z_memory_struct_alloc(z_global_memory(), struct sset_node);
  if (Z_MALLOC_IS_NULL(node))
    return(NULL);

  z_dlink_init(&(node->dirtyq));
  z_dlink_init(&(node->commitq));

  node->mem_data = NULL;
  node->txn_locks = NULL;
  node->block = block;

  node->bufsize = 0;
  node->min_ksize = ~0;
  node->max_ksize = 0;
  node->min_vsize = ~0;
  node->max_vsize = 0;
  return(node);
}

#define __sset_node_requires_balance(node)  \
  ((node)->bufsize >= __SSET_SYNC_SIZE)

#include <zcl/writer.h>
static int __sset_node_mem_search (struct sset_node *node,
                                   const z_bytes_ref_t *key,
                                   int stop_if_has_txn,
                                   struct sset_entry *entry)
{
  z_tree_node_t *tree_node;

  entry->txn = NULL;
  entry->item = NULL;
  entry->block = NULL;

  /* Try to lookup in the Txn-Buffer */
  if ((tree_node = __sset_txn_node_get(node->txn_locks, key)) != NULL) {
    entry->txn = z_container_of(tree_node, struct sset_txn, __node__);
    entry->item = entry->txn->item;
    entry->value = &(entry->item->value);
    if (stop_if_has_txn) return(1);
  }

  /* Try to lookup in the In-Memory Buffer */
  tree_node = __sset_item_node_get(node->mem_data, key);
  if (tree_node != NULL) {
    entry->item = z_container_of(tree_node, struct sset_item, __node__);
    entry->value = &(entry->item->value);
    return(!__sset_item_is_delete_marker(entry->item) || entry->txn != NULL);
  }

  /* Try to lookup in the block */
  if (node->block != NULL && !__sset_block_lookup(node->block, key, &(entry->blk_value))) {
    entry->block = node->block;
    entry->value = &(entry->blk_value);
    return(3);
  }

  /* Not found */
  return(entry->txn != NULL);
}

/* ============================================================================
 *  PRIVATE SSet Node Iter methods
 */
struct sset_node_iter {
  struct sset_txn_iter iter_txn;
  struct sset_mem_iter iter_mem;
  z_bucket_iterator_t  iter_blk;
  z_map_merger_t merger;
};

#define __sset_node_iter_get_refs(iter, key_ref, value_ref)                   \
  z_map_iterator_get_refs((iter)->merger.smallest_iter, key_ref, value_ref)

static void __sset_node_iter_open (struct sset_node_iter *iter,
                                   const struct sset_node *node,
                                   uint64_t txn_id,
                                   const z_byte_slice_t *key,
                                   int include_key)
{
  z_map_merger_open(&(iter->merger));
  iter->merger.skip_equals = 1;

  if (node->txn_locks != NULL) {
    z_map_iterator_t *map_iter = Z_MAP_ITERATOR(&(iter->iter_txn));
    /* TODO: Add support for dirty-reads (not-committed txns) */
    __sset_txn_iter_open(&(iter->iter_txn), node->txn_locks, txn_id);
    z_map_iterator_seek_to(map_iter, key, include_key);
    z_map_merger_add(&(iter->merger), map_iter);
  }

  if (node->mem_data != NULL) {
    z_map_iterator_t *map_iter = Z_MAP_ITERATOR(&(iter->iter_mem));
    __sset_mem_iter_open(&(iter->iter_mem), node->mem_data);
    z_map_iterator_seek_to(map_iter, key, include_key);
    z_map_merger_add(&(iter->merger), map_iter);
  }

  if (node->block != NULL) {
    z_map_iterator_t *map_iter = Z_MAP_ITERATOR(&(iter->iter_blk));
    z_bucket_iterator_open(&(iter->iter_blk),
                           __sset_block_type(node->block),
                           node->block->data,
                           &__sset_block_vtable_refs, node->block);
    z_map_iterator_seek_to(map_iter, key, include_key);
    z_map_merger_add(&(iter->merger), map_iter);
  }
}

static void __sset_node_iter_close (struct sset_node_iter *iter) {
  z_map_merger_close(&(iter->merger));
}

#define __has_delete_marker(current_iter, iter)                         \
  ((current_iter) == Z_MAP_ITERATOR(iter) && (iter)->is_delete_marker)

static const z_map_entry_t *__sset_node_iter_next (struct sset_node_iter *iter) {
  z_map_merger_t *merger = &(iter->merger);
  const z_map_entry_t *entry;
  while ((entry = z_map_merger_next(merger)) != NULL) {
    if (Z_UNLIKELY(entry->is_delete_marker))
      continue;
    return(entry);
  }
  return(NULL);
}

/* ============================================================================
 *  PRIVATE SSet Node Balanceer/Merger methods
 */

struct kprefix {
  uint8_t buffer[128];
  size_t  size;
};

size_t __kprefix_shared (struct kprefix *self,
                         const z_byte_slice_t *key,
                         z_byte_slice_t *prefix_key)
{
  size_t shared = z_memshared(self->buffer, self->size, key->data, key->size);
  Z_ASSERT(key->size <= 128, "Keys > 128 not supported yet");
  z_byte_slice_set(prefix_key, key->data + shared, key->size - shared);
  z_memcpy(self->buffer + shared, prefix_key->data, prefix_key->size);
  self->size = key->size;
  return(shared);
}

/*
 * A B C D [data] E F G
 */
static int __sset_node_balance (struct sset_node *node, z_dlink_node_t *blkseq) {
  struct sset_node_iter iter;
  z_dlink_node_t node_blkseq;
  const z_map_entry_t *entry;
  struct sset_block *block;
  struct kprefix kprefix;

  z_dlink_init(&node_blkseq);

  /*
   * Generate the new blocks from the in-memory data
   * TODO: Verify if the current block overlaps
   */
  __sset_node_iter_open(&iter, node, 0, NULL, 0);
  entry = __sset_node_iter_next(&iter);
  while (entry != NULL) {
    z_byte_slice_t prefix_key;
    kprefix.size = 0;

    block = __sset_block_create();
    if (Z_MALLOC_IS_NULL(block))
      break;

    do {
      size_t shared = __kprefix_shared(&kprefix, &(entry->key), &prefix_key);
      if (__sset_block_add(block, shared, &prefix_key, &(entry->value)))
        break;

      entry = __sset_node_iter_next(&iter);
    } while (entry != NULL);
    __sset_block_finalize(block);
    z_dlink_add_tail(&node_blkseq, &(block->blkseq));
  }
  __sset_node_iter_close(&iter);

  /* handle memory error during block creation */
  if (Z_UNLIKELY(entry != NULL)) {
    z_dlink_for_each_safe_entry(&node_blkseq, block, struct sset_block, blkseq, {
      __sset_block_free(block);
    });
    return(0);
  }

  /* Add blocks to the output list */
  z_dlink_for_each_safe_entry(&node_blkseq, block, struct sset_block, blkseq, {
    z_dlink_move_tail(blkseq, &(block->blkseq));
  });
  return(1);
}

/* ============================================================================
 *  PRIVATE SSet WRITE methods
 */
static raleighsl_errno_t __sset_txn_add (raleighsl_t *fs,
                                         raleighsl_transaction_t *transaction,
                                         raleighsl_object_t *object,
                                         struct sset_node *node,
                                         enum sset_txn_type type,
                                         struct sset_item *item)
{
  raleighsl_sset_t *sset = RALEIGHSL_SSET(object->membufs);
  struct sset_txn *txn;

  /* Allocate Txn-Atom */
  txn = __sset_txn_alloc(__sset_txn_id(transaction), node, type, item);
  if (Z_MALLOC_IS_NULL(txn)) {
    __sset_item_free(item);
    return(RALEIGHSL_ERRNO_NO_MEMORY);
  }

  if (transaction == NULL) {
    /* Attach the txn to the commitq */
    z_dlink_move(&(sset->dirtyq), &(node->dirtyq));
    z_dlink_add_tail(&(node->commitq), &(txn->commitq));
  } else {
    int res;

    /* Add Txn-Atom to the Transaction */
    res = raleighsl_transaction_add(fs, transaction, object, &(txn->__txn_atom__));
    if (Z_UNLIKELY(res)) {
      __sset_txn_free(txn);
      __sset_item_free(item);
      return(RALEIGHSL_ERRNO_NO_MEMORY);
    }

    /* Attach the txn to the commitq */
    z_dlink_add_tail(&(sset->txnq), &(txn->commitq));
  }

  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __sset_txn_update (raleighsl_t *fs,
                                            raleighsl_transaction_t *transaction,
                                            raleighsl_object_t *object,
                                            struct sset_txn *txn,
                                            enum sset_txn_type type,
                                            const z_bytes_ref_t *key,
                                            const z_bytes_ref_t *value)
{
  raleighsl_sset_t *sset = RALEIGHSL_SSET(object->membufs);
  struct sset_item *item = txn->item;
  struct sset_txn *new_txn;

  Z_ASSERT(type != SSET_TXN_UPDATE, "type must be INSERT or REMOVE");

  /*
   * +-----------+-----------+--------------------+
   * | old state | new state | res state          |
   * +-----------+-----------+--------------------+
   * | INSERT    | INSERT    | INSERT             |
   * | INSERT    | UPDATE    | INSERT             |
   * | INSERT    | REMOVE    | REMOVE (del-apply) |
   * +-----------+-----------+--------------------+
   * | UPDATE    | INSERT    | throw key present  |
   * | UPDATE    | UPDATE    | UPDATE             |
   * | UPDATE    | REMOVE    | REMOVE             |
   * +-----------+-----------+--------------------+
   * | REMOVE    | INSERT    | UPDATE             |
   * | REMOVE    | UPDATE    | UPDATE             |
   * | REMOVE    | REMOVE    | throw not exists   |
   * +-----------+-----------+--------------------+
   */
  switch (txn->type) {
    case SSET_TXN_INSERT:
      if (type != SSET_TXN_REMOVE) {
        type = SSET_TXN_INSERT;
        item = NULL;
      }
      break;
    case SSET_TXN_UPDATE:
      Z_ASSERT(type != SSET_TXN_INSERT, "key already present");
      break;
    case SSET_TXN_REMOVE:
      if (type != SSET_TXN_REMOVE) {
        type = SSET_TXN_UPDATE;
        item = NULL;
      }
      break;
  }

  if (item == NULL) {
    Z_ASSERT(type != SSET_TXN_REMOVE, "expected item to remove");

    /* Allocate the new item */
    item = __sset_item_alloc(key, value, 0);
    if (Z_MALLOC_IS_NULL(item))
      return(RALEIGHSL_ERRNO_NO_MEMORY);
  }

  /* Allocate Txn-Atom */
  new_txn = __sset_txn_alloc(__sset_txn_id(transaction), txn->node, type, item);
  if (Z_MALLOC_IS_NULL(new_txn)) {
    if (type != SSET_TXN_REMOVE) {
      __sset_item_free(item);
    }
    return(RALEIGHSL_ERRNO_NO_MEMORY);
  }

  /* Set the parent-txn */
  new_txn->parent = txn;

  /* Attach the new_txn to the commitq */
  if (transaction == NULL) {
    z_dlink_move(&(sset->dirtyq), &(txn->node->dirtyq));
    z_dlink_add_tail(&(txn->node->commitq), &(new_txn->commitq));
  } else {
    if (type == SSET_TXN_REMOVE) {
      raleighsl_transaction_remove(fs, transaction, object, &(txn->__txn_atom__));
    } else {
      raleighsl_transaction_replace(fs, transaction, object,
                                    &(txn->__txn_atom__), &(new_txn->__txn_atom__));
    }

    z_dlink_add_tail(&(sset->txnq), &(new_txn->commitq));
  }

  return(RALEIGHSL_ERRNO_NONE);
}

static void __sset_txn_apply (raleighsl_sset_t *sset, struct sset_txn *txn) {
  if (txn->txn_id > 0) {
    __sset_txn_detach(txn);
  }

  switch (txn->type) {
    case SSET_TXN_INSERT:
    case SSET_TXN_UPDATE:
      /* Update node stats */
      txn->node->bufsize += __sset_item_size(txn->item);
      z_min(txn->node->min_ksize, txn->item->key.slice.size);
      z_max(txn->node->max_ksize, txn->item->key.slice.size);
      z_min(txn->node->min_vsize, txn->item->value.slice.size);
      z_max(txn->node->max_vsize, txn->item->value.slice.size);

      /* Attach item */
      __sset_item_attach(&(txn->node->mem_data), txn->item);
      break;
    case SSET_TXN_REMOVE: {
      z_tree_node_t *node;
      if ((node = __sset_item_detach(&(txn->node->mem_data), &(txn->item->key))) != NULL) {
        __sset_item_node_free(sset, node);
        __sset_item_free(txn->item);
      } else {
        /* Attach item (replaces the old one) */
        __sset_item_attach(&(txn->node->mem_data), txn->item);
      }
      break;
    }
  }

  __sset_txn_free(txn);
}

static void __sset_txn_revert (raleighsl_sset_t *sset, struct sset_txn *txn) {
  /* Remove the TXN from the node */
  __sset_txn_detach(txn);
  __sset_item_free(txn->item);
  __sset_txn_free(txn);
}

static void __sset_txn_commit (raleighsl_sset_t *sset, struct sset_txn *txn) {
  Z_ASSERT(txn->txn_id > 0, "Default transaction are handld as committed apply");

  if (txn->parent != NULL) {
    __sset_txn_revert(sset, txn->parent);
    if (txn->type == SSET_TXN_REMOVE) {
      __sset_txn_free(txn);
    }
  } else {
    __sset_txn_attach(txn);
  }
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
  struct sset_entry entry;
  struct sset_node *node;

  /* Lookup key-node */
  node = __sset_node_lookup(sset, key);
  Z_ASSERT(node != NULL, "Unable to find a node");

  /* Lookup the key */
  if (__sset_node_mem_search(node, key, 1, &entry)) {
    /* if the item-txn is owned by someone else, fail the current-txn */
    if (__sset_txn_key_locked(entry.txn, transaction))
      return(RALEIGHSL_ERRNO_TXN_LOCKED_KEY);

    /* updates are not allowed, and the key is already in */
    if (!allow_update)
      return(RALEIGHSL_ERRNO_DATA_KEY_EXISTS);

    /* if I'm the owner of the TXN, just replace the value */
    if (Z_UNLIKELY(entry.txn != NULL))
      return(__sset_txn_update(fs, transaction, object, entry.txn, SSET_TXN_INSERT, key, value));
  }

  /* Allocate the new item */
  entry.item = __sset_item_alloc(key, value, 0);
  if (Z_MALLOC_IS_NULL(entry.item))
    return(RALEIGHSL_ERRNO_NO_MEMORY);

  /* Add to the transaction */
  object->requires_balancing = __sset_node_requires_balance(node);
  return(__sset_txn_add(fs, transaction, object, node, SSET_TXN_INSERT, entry.item));
}

raleighsl_errno_t raleighsl_sset_update (raleighsl_t *fs,
                                         raleighsl_transaction_t *transaction,
                                         raleighsl_object_t *object,
                                         const z_bytes_ref_t *key,
                                         const z_bytes_ref_t *value,
                                         z_bytes_ref_t *old_value)
{
  raleighsl_errno_t errno;

  /* TODO: Just add, the merge will take care of replacing the old item */

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
  struct sset_entry entry;
  struct sset_node *node;

  /* Lookup key-node */
  node = __sset_node_lookup(sset, key);
  Z_ASSERT(node != NULL, "Unable to find a node");

  /* Lookup the key - TODO: Search just in-memory */
  if (!__sset_node_mem_search(node, key, 1, &entry)) {
    /*
     * TODO: May be on disk?
     *       if we don't need to validate the key, just add it to the rm_keys
     *       otherwise, send I/O Request and return an IO_RETRY.
     */
    return(RALEIGHSL_ERRNO_DATA_KEY_NOT_FOUND);
  }

  /* if the item-txn is owned by someone else, fail the current-txn */
  if (__sset_txn_key_locked(entry.txn, transaction))
    return(RALEIGHSL_ERRNO_TXN_LOCKED_KEY);

  /* Acquire value for the user */
  z_bytes_ref_acquire(value, entry.value);

  /* if I'm the owner of the TXN, just replace the value */
  if (Z_UNLIKELY(entry.txn != NULL))
    return(__sset_txn_update(fs, transaction, object, entry.txn, SSET_TXN_REMOVE, NULL, NULL));

  /* Allocate the new rm-key item */
  entry.item = __sset_item_alloc(key, NULL, 1);
  if (Z_MALLOC_IS_NULL(entry.item))
    return(RALEIGHSL_ERRNO_NO_MEMORY);

  /* Add to the transaction */
  return(__sset_txn_add(fs, transaction, object, node, SSET_TXN_REMOVE, entry.item));
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
  struct sset_entry entry;
  struct sset_node *node;

  /* Lookup key-node */
  node = __sset_node_lookup(sset, key);
  Z_ASSERT(node != NULL, "Unable to find a node");

  /* Lookup the key - TODO: Search just in-memory */
  if (!__sset_node_mem_search(node, key, 0, &entry)) {
    /* TODO: May be on disk? Send I/O Request and return an IO_RETRY */
    return(RALEIGHSL_ERRNO_DATA_KEY_NOT_FOUND);
  }

  if (__sset_txn_is_current(entry.txn, transaction)) {
    /* Acquire value for the user */
    z_bytes_ref_acquire(value, entry.value);
    return(RALEIGHSL_ERRNO_NONE);
  }

  if (entry.txn != NULL && entry.item == entry.txn->item)
    return(RALEIGHSL_ERRNO_DATA_KEY_NOT_FOUND);

  /* Acquire value for the user */
  z_bytes_ref_acquire(value, entry.value);

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
  const z_byte_slice_t *key_slice;
  const struct sset_node *node;
  z_tree_iter_t iter_node;

  z_tree_iter_open(&iter_node, sset->root);
  if (z_bytes_ref_is_empty(key)) {
    key_slice = NULL;
    node = __sset_node_from_tree(z_tree_iter_begin(&iter_node));
  } else {
    key_slice = z_bytes_slice(key);
    node = __sset_node_from_tree(z_tree_iter_seek_le(&iter_node, __sset_node_key_compare, key, NULL));
  }

  while (node != NULL && count > 0) {
    struct sset_node_iter iter_data;

    __sset_node_iter_open(&iter_data, node, __sset_txn_id(transaction), key_slice, include_key);
    while (count > 0 && __sset_node_iter_next(&iter_data) != NULL) {
      z_bytes_ref_t *value_ref = z_array_push_back(values);
      z_bytes_ref_t *key_ref = z_array_push_back(keys);
      __sset_node_iter_get_refs(&iter_data, key_ref, value_ref);
      --count;
    }
    __sset_node_iter_close(&iter_data);

    key_slice = NULL;
    node = __sset_node_from_tree(z_tree_iter_next(&iter_node));
  }

  z_tree_iter_close(&iter_node);
  return(RALEIGHSL_ERRNO_NONE);
}

#if 0
#include <zcl/writer.h>
void __sset_node_dump(raleighsl_sset_t *sset, const struct sset_node *node) {
  struct sset_node_iter iter_data;
  const z_map_entry_t *entry;

  __sset_node_iter_open(&iter_data, node, 0, NULL, 0);
  while ((entry = __sset_node_iter_next(&iter_data)) != NULL) {
    fprintf(stderr, "key:");
    z_dump_byte_slice(stderr, &(entry->key));
    fprintf(stderr, " value:");
    z_dump_byte_slice(stderr, &(entry->value));
    fprintf(stderr, "\n");
  }
  __sset_node_iter_close(&iter_data);
}

void __sset_dump(raleighsl_sset_t *sset, const struct sset_node *node) {
  const struct sset_node *node;
  fprintf(stderr, "============================================================\n");
  fprintf(stderr, " DUMP NODES\n");
  fprintf(stderr, "============================================================\n");
  z_tree_iter_t iter_node;
  z_tree_iter_open(&iter_node, sset->root);
  node = __sset_node_from_tree(z_tree_iter_begin(&iter_node));
  while (node != NULL) {
    fprintf(stderr, "=================== NODE %p ===================\n", node);
    __sset_node_dump(sset, node);
    node = __sset_node_from_tree(z_tree_iter_next(&iter_node));
  }
}
#endif

/* ============================================================================
 *  SSet Object Plugin
 */
static raleighsl_errno_t __object_create (raleighsl_t *fs,
                                          raleighsl_object_t *object)
{
  raleighsl_sset_t *sset;
  struct sset_node *node;

  Z_LOG_INFO("sset-item  %ld", z_sizeof(struct sset_item));
  Z_LOG_INFO("sset-txn   %ld", z_sizeof(struct sset_txn));
  Z_LOG_INFO("sset-txn-t %ld", z_sizeof(enum sset_txn_type));
  Z_LOG_INFO("sset-block %ld", z_sizeof(struct sset_block));
  Z_LOG_INFO("sset-node  %ld", z_sizeof(struct sset_node));
  Z_LOG_INFO("sset-entry %ld", z_sizeof(struct sset_entry));
  Z_LOG_INFO("sset       %ld", z_sizeof(raleighsl_sset_t));

  sset = z_memory_struct_alloc(z_global_memory(), raleighsl_sset_t);
  if (Z_MALLOC_IS_NULL(sset))
    return(RALEIGHSL_ERRNO_NO_MEMORY);

  /* Initialize Root Node */
  node = __sset_node_alloc(NULL);
  if (Z_MALLOC_IS_NULL(node)) {
    z_memory_struct_free(z_global_memory(), raleighsl_sset_t, sset);
    return(RALEIGHSL_ERRNO_NO_MEMORY);
  }

  /* Attach the Root node */
  sset->root = NULL;
  __sset_node_attach(sset, node);

  /* Initialize the dirty-queue */
  z_dlink_init(&(sset->txnq));
  z_dlink_init(&(sset->dirtyq));

  z_dlink_init(&(sset->rm_nodes));
  z_dlink_init(&(sset->add_nodes));

  object->membufs = sset;
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __object_close (raleighsl_t *fs,
                                         raleighsl_object_t *object)
{
  raleighsl_sset_t *sset = RALEIGHSL_SSET(object->membufs);
  z_tree_node_clear(&__sset_node_tree_info, sset->root, NULL);
  z_memory_struct_free(z_global_memory(), raleighsl_sset_t, sset);
  return(RALEIGHSL_ERRNO_NONE);
}

static void __object_apply (raleighsl_t *fs,
                            raleighsl_object_t *object,
                            raleighsl_txn_atom_t *atom)
{
  struct sset_txn *txn = z_container_of(atom, struct sset_txn, __txn_atom__);
  raleighsl_sset_t *sset = RALEIGHSL_SSET(object->membufs);

  z_dlink_move(&(sset->dirtyq), &(txn->node->dirtyq));
  z_dlink_add_tail(&(txn->node->commitq), &(txn->commitq));
}

static void __object_revert (raleighsl_t *fs,
                             raleighsl_object_t *object,
                             raleighsl_txn_atom_t *atom)
{
  struct sset_txn *txn = z_container_of(atom, struct sset_txn, __txn_atom__);
  raleighsl_sset_t *sset = RALEIGHSL_SSET(object->membufs);
  __sset_txn_revert(sset, txn);
}

static raleighsl_errno_t __object_commit (raleighsl_t *fs,
                                          raleighsl_object_t *object)
{
  raleighsl_sset_t *sset = RALEIGHSL_SSET(object->membufs);
  struct sset_node *node;
  struct sset_txn *txn;

  /* Detach old nodes */
  z_dlink_del_for_each_entry(&(sset->rm_nodes), node, struct sset_node, commitq, {
    z_tree_node_t *dnode = __sset_node_detach(sset, node);
    Z_ASSERT(dnode == &(node->__node__), "NOT DETACHD %p", dnode);
    __sset_node_free(sset, node);
  });

  /* Attach new nodes */
  z_dlink_del_for_each_entry(&(sset->add_nodes), node, struct sset_node, commitq, {
    __sset_node_attach(sset, node);
    object->requires_balancing = 0;
  });

  /* Apply the pending txn-atom write */
  z_dlink_del_for_each_entry(&(sset->txnq), txn, struct sset_txn, commitq, {
    __sset_txn_commit(sset, txn);
  });

  /* TODO: Write to Journal */
  Z_LOG_WARN("TODO: Write to Journal");

  /* Apply commits - [Point of No Return] */
  z_dlink_del_for_each_entry(&(sset->dirtyq), node, struct sset_node, dirtyq, {
    z_dlink_del_for_each_entry(&(node->commitq), txn, struct sset_txn, commitq, {
      __sset_txn_apply(sset, txn);
    });
  });

  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __object_balance (raleighsl_t *fs,
                                           raleighsl_object_t *object)
{
  raleighsl_sset_t *sset = RALEIGHSL_SSET(object->membufs);
  struct sset_block *block;
  z_tree_iter_t iter_node;
  struct sset_node *node;
  z_dlink_node_t blkseq;

  Z_ASSERT(z_dlink_is_empty(&(sset->txnq)), "there are pending commits for the txns");
  Z_ASSERT(z_dlink_is_empty(&(sset->dirtyq)), "there are pending commits for the nodes");

  /* Build new blocks */
  z_dlink_init(&blkseq);
  z_tree_iter_open(&iter_node, sset->root);
  node = __sset_node_from_tree(z_tree_iter_begin(&iter_node));
  while (node != NULL) {
    if (__sset_node_requires_balance(node)) {
      if (__sset_node_balance(node, &blkseq)) {
        z_dlink_add_tail(&(sset->rm_nodes), &(node->commitq));
      }
    }
    node = __sset_node_from_tree(z_tree_iter_next(&iter_node));
  }
  z_tree_iter_close(&iter_node);

  /* quick exit, nothing to do */
  if (z_dlink_is_empty(&blkseq)) {
    object->requires_balancing = 0;
    return(RALEIGHSL_ERRNO_NONE);
  }

  /* TODO: Merge small blocks */
  Z_LOG_TRACE("TODO: BALANCE!");

  /* Build new nodes */
  z_dlink_del_for_each_entry(&blkseq, block, struct sset_block, blkseq, {
    node = __sset_node_alloc(block);
    if (Z_MALLOC_IS_NULL(node))
      break;
    z_dlink_add_tail(&(sset->add_nodes), &(node->commitq));
  });

  if (Z_UNLIKELY(z_dlink_is_not_empty(&blkseq))) {
    /* Remove new blocks */
    z_dlink_del_for_each_entry(&blkseq, block, struct sset_block, blkseq, {
      __sset_block_free(block);
    });
    /* Remove new nodes */
    z_dlink_del_for_each_entry(&(sset->add_nodes), node, struct sset_node, commitq, {
      __sset_node_free(sset, node);
    });
    /* Reset rm_nodes list */
    z_dlink_init(&(sset->rm_nodes));
    return(RALEIGHSL_ERRNO_NO_MEMORY);
  }

  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __object_sync (raleighsl_t *fs,
                                        raleighsl_object_t *object)
{
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
  .unlink   = NULL,

  .apply    = __object_apply,
  .revert   = __object_revert,
  .commit   = __object_commit,

  .balance  = __object_balance,
  .sync     = __object_sync,
};
