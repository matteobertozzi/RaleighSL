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
#include <zcl/debug.h>

#include "sset.h"

#define RALEIGHFS_SSET(x)                 Z_CAST(raleighfs_sset_t, x)
#define __SSET_ENTRY(x)                   Z_CAST(struct sset_entry, x)

typedef struct raleighfs_sset {
  z_skip_list_t skip_list;
  z_skip_list_t txn_locks;
} raleighfs_sset_t;

struct sset_entry {
  z_bytes_t *key;
  z_bytes_t *value;
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
#define __sset_entry_get_func(func, sset, key)                                \
  (Z_CAST(struct sset_entry, func(&((sset)->skip_list),                       \
                                  __sset_entry_key_compare, key)))

#define __sset_entry_lookup(sset, key)                                        \
  __sset_entry_get_func(z_skip_list_get_custom, sset, key)

#define __sset_entry_remove(fs, key)                                          \
  __sset_entry_get_func(z_skip_list_remove_custom, sset, key)

static int __sset_entry_compare (void *udata, const void *a, const void *b) {
  const struct sset_entry *ea = (const struct sset_entry *)a;
  const struct sset_entry *eb = (const struct sset_entry *)b;
  return(z_bytes_compare(ea->key, eb->key));
}

static int __sset_entry_key_compare (void *udata, const void *a, const void *key) {
  const struct sset_entry *entry = (const struct sset_entry *)a;
  return(z_bytes_compare(entry->key, Z_BYTES(key)));
}

static void __sset_entry_free (void *udata, void *obj) {
  struct sset_entry *entry = __SSET_ENTRY(obj);
  z_bytes_free(entry->key);
  z_bytes_free(entry->value);
  z_memory_struct_free(z_global_memory(), struct sset_entry, entry);
}

static struct sset_entry *__sset_entry_alloc (raleighfs_sset_t *sset,
                                              z_bytes_t *key,
                                              z_bytes_t *value)
{
  z_memory_t *memory = z_global_memory();
  struct sset_entry *entry;

  entry = z_memory_struct_alloc(memory, struct sset_entry);
  if (Z_MALLOC_IS_NULL(entry))
    return(NULL);

  entry->key = z_bytes_acquire(key);
  entry->value = z_bytes_acquire(value);
  return(entry);
}

static void __sset_entry_set_value (raleighfs_sset_t *sset,
                                    struct sset_entry *entry,
                                    z_bytes_t *value)
{
  z_bytes_free(entry->value);
  entry->value = z_bytes_acquire(value);
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
  return(z_bytes_compare(txn_a->entry->key, txn_b->entry->key));
}

static int __sset_txn_key_compare (void *udata, const void *a, const void *key) {
  struct sset_txn *txn = (struct sset_txn *)a;
  return(z_bytes_compare(txn->entry->key, Z_BYTES(key)));
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
  z_memory_struct_free(z_global_memory(), struct sset_txn, obj);
}

static struct sset_txn *__sset_txn_has_lock (raleighfs_sset_t *sset, const z_bytes_t *key) {
  if (sset->txn_locks.size > 0)
    return(__sset_txn_lookup(sset, key));
  return(NULL);
}

static raleighfs_errno_t __sset_txn_add (raleighfs_t *fs,
                                         raleighfs_object_t *object,
                                         raleighfs_transaction_t *transaction,
                                         enum sset_txn_type type,
                                         struct sset_entry *entry)
{
  raleighfs_sset_t *sset = RALEIGHFS_SSET(object->membufs);
  struct sset_txn *txn;

  txn = __sset_txn_alloc(raleighfs_txn_id(transaction), type, entry);
  if (Z_MALLOC_IS_NULL(txn)) {
    __sset_entry_free(sset, entry);
    return(RALEIGHFS_ERRNO_NO_MEMORY);
  }

  if (z_skip_list_put(&(sset->txn_locks), txn)) {
    __sset_txn_free(sset, txn);
    __sset_entry_free(sset, entry);
    return(RALEIGHFS_ERRNO_NO_MEMORY);
  }

  if (raleighfs_transaction_add(fs, transaction, object, txn)) {
    z_skip_list_rollback(&(sset->txn_locks));
    __sset_entry_free(sset, entry);
    return(RALEIGHFS_ERRNO_NO_MEMORY);
  }

  z_skip_list_commit(&(sset->txn_locks));
  return(RALEIGHFS_ERRNO_NONE);
}

static void __sset_txn_remove (raleighfs_sset_t *sset,
                               struct sset_txn *txn,
                               int release_entity)
{
  __sset_txn_get_func(z_skip_list_remove_custom, sset, txn->entry->key);
  if (release_entity)
    __sset_entry_free(sset, txn->entry);
  txn->entry = NULL;
  z_skip_list_commit(&(sset->txn_locks));
}

/* ============================================================================
 *  PRIVATE  SSet Txn Apply methods
 */
static raleighfs_errno_t __sset_apply_insert (raleighfs_sset_t *sset,
                                              struct sset_entry *entry)
{
  /* Add directly to the table */
  if (z_skip_list_put(&(sset->skip_list), entry)) {
    __sset_entry_free(sset, entry);
    return(RALEIGHFS_ERRNO_NO_MEMORY);
  }

  return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __sset_apply_remove (raleighfs_sset_t *sset,
                                              const z_bytes_t *key,
                                              z_bytes_t **value)
{
  struct sset_entry *entry;

  if ((entry = __sset_entry_remove(sset, key)) == NULL)
    return(RALEIGHFS_ERRNO_DATA_KEY_NOT_FOUND);

  if (value != NULL)
    *value = z_bytes_acquire(entry->value);

  return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __sset_apply_update (raleighfs_sset_t *sset,
                                              struct sset_entry *entry)
{
  raleighfs_errno_t errno;

  /* Remove the old value */
  if ((errno = __sset_apply_remove(sset, entry->key, NULL)))
    return(errno);

  /* Add directly to the table */
  return(__sset_apply_insert(sset, entry));
}

/* ============================================================================
 *  PUBLIC SSet WRITE methods
 */
raleighfs_errno_t raleighfs_sset_insert (raleighfs_t *fs,
                                         raleighfs_transaction_t *transaction,
                                         raleighfs_object_t *object,
                                         int allow_update,
                                         z_bytes_t *key,
                                         z_bytes_t *value)
{
  raleighfs_sset_t *sset = RALEIGHFS_SSET(object->membufs);
  struct sset_entry *entry;
  struct sset_txn *txn;

  /* Verify that no other transaction is holding the key-lock */
  txn = __sset_txn_has_lock(sset, key);
  if (txn != NULL && (transaction != NULL && raleighfs_txn_id(transaction) != txn->txn_id))
    return(RALEIGHFS_ERRNO_TXN_LOCKED_KEY);

  /* Update Transaction Value */
  if (txn != NULL) {
    Z_ASSERT(txn->txn_id == raleighfs_txn_id(transaction), "TXN-ID Mismatch");
    switch (txn->type) {
      case SSET_TXN_INSERT:
        __sset_entry_set_value(sset, txn->entry, value);
        break;
      case SSET_TXN_UPDATE:
        return(RALEIGHFS_ERRNO_DATA_KEY_EXISTS);
        break;
      case SSET_TXN_REMOVE:
        __sset_entry_set_value(sset, txn->entry, value);
        txn->type = SSET_TXN_UPDATE;
        break;
    }
    return(RALEIGHFS_ERRNO_NONE);
  }

  if (!allow_update && (entry = __sset_entry_lookup(sset, key)) != NULL)
    return(RALEIGHFS_ERRNO_DATA_KEY_EXISTS);

  /* Allocate the new entry */
  entry = __sset_entry_alloc(sset, key, value);
  if (Z_MALLOC_IS_NULL(entry)) {
    return(RALEIGHFS_ERRNO_NO_MEMORY);
  }

  /* Add to the transaction */
  if (transaction != NULL) {
    return(__sset_txn_add(fs, object, transaction, SSET_TXN_INSERT, entry));
  }

  /* Add directly to the table */
  return(__sset_apply_insert(sset, entry));
}

raleighfs_errno_t raleighfs_sset_update (raleighfs_t *fs,
                                         raleighfs_transaction_t *transaction,
                                         raleighfs_object_t *object,
                                         z_bytes_t *key,
                                         z_bytes_t *value,
                                         z_bytes_t **old_value)
{
  raleighfs_errno_t errno;

  if ((errno = raleighfs_sset_remove(fs, transaction, object, key, old_value)))
    return(errno);

  if ((errno = raleighfs_sset_insert(fs, transaction, object, 1, key, value)))
    return(errno);

  return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_errno_t raleighfs_sset_remove (raleighfs_t *fs,
                                         raleighfs_transaction_t *transaction,
                                         raleighfs_object_t *object,
                                         const z_bytes_t *key,
                                         z_bytes_t **value)
{
  raleighfs_sset_t *sset = RALEIGHFS_SSET(object->membufs);
  struct sset_entry *entry;
  struct sset_txn *txn;

  /* Verify that no other transaction is holding the key-lock */
  txn = __sset_txn_has_lock(sset, key);
  if (txn != NULL && (transaction != NULL && raleighfs_txn_id(transaction) != txn->txn_id))
    return(RALEIGHFS_ERRNO_TXN_LOCKED_KEY);

  if (txn != NULL) {
    Z_ASSERT(txn->txn_id == raleighfs_txn_id(transaction), "TXN-ID Mismatch");
    switch (txn->type) {
      case SSET_TXN_INSERT:
        __sset_txn_remove(sset, txn, 1);
        break;
      case SSET_TXN_UPDATE:
      case SSET_TXN_REMOVE:
        txn->type = SSET_TXN_REMOVE;
        break;
    }
    return(RALEIGHFS_ERRNO_NONE);
  }

  /* Add to the transaction */
  if (transaction != NULL) {
    if ((entry = __sset_entry_lookup(sset, key)) == NULL)
      return(RALEIGHFS_ERRNO_DATA_KEY_NOT_FOUND);

    *value = z_bytes_acquire(entry->value);

    return(__sset_txn_add(fs, object, transaction, SSET_TXN_REMOVE, entry));
  }

  /* Apply directly to the table */
  return(__sset_apply_remove(sset, key, value));
}

/* ============================================================================
 *  PUBLIC SSet READ methods
 */
raleighfs_errno_t raleighfs_sset_get (raleighfs_t *fs,
                                      raleighfs_transaction_t *transaction,
                                      raleighfs_object_t *object,
                                      const z_bytes_t *key,
                                      z_bytes_t **value)
{
  raleighfs_sset_t *sset = RALEIGHFS_SSET(object->membufs);
  struct sset_entry *entry;
  struct sset_txn *txn;

  if (transaction != NULL) {
    txn = __sset_txn_has_lock(sset, key);
    if (txn != NULL && txn->txn_id == raleighfs_txn_id(transaction)) {
      entry = (txn->type == SSET_TXN_REMOVE) ? NULL : txn->entry;
    } else {
      entry = __sset_entry_lookup(sset, key);
    }
  } else {
    entry = __sset_entry_lookup(sset, key);
  }

  if (entry == NULL)
    return(RALEIGHFS_ERRNO_DATA_KEY_NOT_FOUND);

  *value = z_bytes_acquire(entry->value);
  return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_errno_t raleighfs_sset_scan (raleighfs_t *fs,
                                       raleighfs_transaction_t *transaction,
                                       raleighfs_object_t *object,
                                       const z_bytes_t *key,
                                       int include_key,
                                       size_t count,
                                       z_array_t *keys,
                                       z_array_t *values)
{
  raleighfs_sset_t *sset = RALEIGHFS_SSET(object->membufs);
  z_skip_list_iterator_t iter;
  struct sset_entry *entry;

  z_iterator_open(&iter, &(sset->skip_list));

  if (key == NULL) {
    entry = __SSET_ENTRY(z_iterator_begin(&iter));
  } else {
    entry = __SSET_ENTRY(z_iterator_seek(&iter, __sset_entry_key_compare, key));
    if (!include_key) {
      entry = __SSET_ENTRY(z_iterator_next(&iter));
    }
  }

  while (entry != NULL && count > 0) {
    if (transaction != NULL) {
      struct sset_txn *txn = __sset_txn_has_lock(sset, key);
      if (txn != NULL && raleighfs_txn_id(transaction) == txn->txn_id) {
        if (txn->type == SSET_TXN_REMOVE)
          continue;
        entry = txn->entry;
      }
    }
    z_array_push_back_ptr(keys, z_bytes_acquire(entry->key));
    z_array_push_back_ptr(values, z_bytes_acquire(entry->value));
    entry = __SSET_ENTRY(z_iterator_next(&iter));
    count--;
  }

  z_iterator_close(&iter);
  return(RALEIGHFS_ERRNO_NONE);
}

/* ============================================================================
 *  SSet Object Plugin
 */
static raleighfs_errno_t __object_create (raleighfs_t *fs,
                                          raleighfs_object_t *object)
{
  raleighfs_sset_t *sset;

  sset = z_memory_struct_alloc(z_global_memory(), raleighfs_sset_t);
  if (Z_MALLOC_IS_NULL(sset))
    return(RALEIGHFS_ERRNO_NO_MEMORY);

  if (z_skip_list_alloc(&(sset->skip_list),
                        __sset_entry_compare, __sset_entry_free,
                        sset, (unsigned int)object) == NULL)
  {
    z_memory_struct_free(z_global_memory(), raleighfs_sset_t, sset);
    return(RALEIGHFS_ERRNO_NO_MEMORY);
  }

  if (z_skip_list_alloc(&(sset->txn_locks),
                        __sset_txn_compare, __sset_txn_free,
                        sset, (unsigned int)object) == NULL)
  {
    z_skip_list_free(&(sset->skip_list));
    z_memory_struct_free(z_global_memory(), raleighfs_sset_t, sset);
    return(RALEIGHFS_ERRNO_NO_MEMORY);
  }

  object->membufs = sset;
  return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __object_close (raleighfs_t *fs,
                                         raleighfs_object_t *object)
{
  raleighfs_sset_t *sset = RALEIGHFS_SSET(object->membufs);
  z_skip_list_free(&(sset->txn_locks));
  z_skip_list_free(&(sset->skip_list));
  z_memory_struct_free(z_global_memory(), raleighfs_sset_t, sset);
  return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __object_apply (raleighfs_t *fs,
                                         raleighfs_object_t *object,
                                         void *mutation)
{
  raleighfs_sset_t *sset = RALEIGHFS_SSET(object->membufs);
  struct sset_txn *txn = (struct sset_txn *)mutation;
  raleighfs_errno_t errno;

  switch (txn->type) {
    case SSET_TXN_INSERT:
      errno = __sset_apply_insert(sset, txn->entry);
      break;
    case SSET_TXN_UPDATE:
      errno = __sset_apply_update(sset, txn->entry);
      break;
    case SSET_TXN_REMOVE:
      errno = __sset_apply_remove(sset, txn->entry->key, NULL);
      break;
    default:
      errno = RALEIGHFS_ERRNO_NOT_IMPLEMENTED;
      Z_LOG_WARN("Unknown sset_txn->type=%d", txn->type);
      break;
  }

  __sset_txn_remove(sset, txn, 0);
  z_skip_list_commit(&(sset->skip_list));
  return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __object_revert (raleighfs_t *fs,
                                          raleighfs_object_t *object,
                                          void *mutation)
{
  raleighfs_sset_t *sset = RALEIGHFS_SSET(object->membufs);
  struct sset_txn *txn = (struct sset_txn *)mutation;
  __sset_txn_remove(sset, txn, 1);
  return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __object_commit (raleighfs_t *fs,
                                          raleighfs_object_t *object)
{
  raleighfs_sset_t *sset = RALEIGHFS_SSET(object->membufs);
  z_skip_list_commit(&(sset->skip_list));
  return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __object_rollback (raleighfs_t *fs,
                                            raleighfs_object_t *object)
{
  raleighfs_sset_t *sset = RALEIGHFS_SSET(object->membufs);
  z_skip_list_rollback(&(sset->skip_list));
  return(RALEIGHFS_ERRNO_NONE);
}

#if 0
#include <zcl/btree.h>
static void __entry_to_item (const struct sset_entry *entry,
                             const z_byte_slice_t *kprev,
                             z_btree_item_t *item)
{
  uint32_t kshared;
  kshared = z_memshared(kprev->data, kprev->length,
                        z_bytes_data(entry->key), z_bytes_length(entry->key));
  item->key = z_bytes_data(entry->key);
  item->kprefix = kshared;
  item->ksize = z_bytes_length(entry->key) - kshared;
  item->value = z_bytes_data(entry->value);
  item->vsize = z_bytes_length(entry->value);
}

static raleighfs_errno_t __object_sync (raleighfs_t *fs,
                                        raleighfs_object_t *object)
{
  raleighfs_sset_t *sset = RALEIGHFS_SSET(object->membufs);
  const z_vtable_btree_node_t *vtable = &z_btree_vnode;
  z_skip_list_iterator_t iter;
  struct sset_entry *entry;

  z_iterator_open(&iter, &(sset->skip_list));
  entry = __SSET_ENTRY(z_iterator_begin(&iter));
  while (entry != NULL) {
    z_byte_slice_t kprev;
    uint8_t node[8192];

    z_byte_slice_clear(&kprev);
    vtable->create(node, sizeof(node));
    do {
      z_btree_item_t item;
      __entry_to_item(entry, &kprev, &item);
      if (vtable->append(node, &item))
        break;

      z_byte_slice_set(&kprev, z_bytes_data(entry->key), z_bytes_length(entry->key));
      entry = __SSET_ENTRY(z_iterator_next(&iter));
    } while (entry != NULL);
    vtable->close(node);
  }

  z_iterator_close(&iter);
  return(RALEIGHFS_ERRNO_NONE);
}
#endif
const raleighfs_object_plug_t raleighfs_object_sset = {
  .info = {
    .type = RALEIGHFS_PLUG_TYPE_OBJECT,
    .description = "Sorted-Set Object",
    .label       = "sset",
  },

  .create   = __object_create,
  .open     = NULL,
  .close    = __object_close,
  .commit   = __object_commit,
  .rollback = __object_rollback,
  .sync     = NULL,
  .unlink   = NULL,
  .apply    = __object_apply,
  .revert   = __object_revert,
};
