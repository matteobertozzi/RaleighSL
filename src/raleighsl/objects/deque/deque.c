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
#include <zcl/debug.h>
#include <zcl/dlink.h>

#include "deque.h"

#define RALEIGHSL_DEQUE(x)                 Z_CAST(raleighsl_deque_t, x)

struct deque_entry {
  z_dlink_node_t node;
  z_bytes_ref_t  data;
};

typedef struct raleighsl_deque {
  z_dlink_node_t data;
  z_dlink_node_t pending_front;
  z_dlink_node_t pending_back;
  z_dlink_node_t *removed_front;
  z_dlink_node_t *removed_back;
  uint64_t txn_id_front;
  uint64_t txn_id_back;
} raleighsl_deque_t;

/* ============================================================================
 *  PRIVATE Deque Entry methods
 */
static void __deque_entry_free (void *udata, void *obj) {
  struct deque_entry *entry = (struct deque_entry *)obj;
  z_bytes_ref_release(&(entry->data));
  z_memory_struct_free(z_global_memory(), struct deque_entry, entry);
}

static struct deque_entry *__deque_entry_alloc (raleighsl_deque_t *deque,
                                                const z_bytes_ref_t *data)
{
  z_memory_t *memory = z_global_memory();
  struct deque_entry *entry;

  entry = z_memory_struct_alloc(memory, struct deque_entry);
  if (Z_MALLOC_IS_NULL(entry))
    return(NULL);

  z_dlink_init(&(entry->node));
  z_bytes_ref_acquire(&(entry->data), data);
  return(entry);
}

/* ============================================================================
 *  PUBLIC Deque WRITE methods
 */
raleighsl_errno_t raleighsl_deque_push (raleighsl_t *fs,
                                        raleighsl_transaction_t *transaction,
                                        raleighsl_object_t *object,
                                        int push_front,
                                        const z_bytes_ref_t *data)
{
  raleighsl_deque_t *deque = RALEIGHSL_DEQUE(object->membufs);
  z_dlink_node_t *pending_deque;
  struct deque_entry *entry;
  uint64_t *current_txn_id;
  uint64_t txn_id;

  if (push_front) {
    pending_deque = &(deque->pending_front);
    current_txn_id = &(deque->txn_id_front);
  } else {
    pending_deque = &(deque->pending_back);
    current_txn_id = &(deque->txn_id_back);
  }

  /* Verify that no other transaction is holding the operation-lock */
  txn_id = (transaction != NULL) ? raleighsl_txn_id(transaction) : 0;
  if (*current_txn_id > 0 && *current_txn_id != txn_id)
    return(RALEIGHSL_ERRNO_TXN_LOCKED_OPERATION);

  /* Allocate the new entry */
  entry = __deque_entry_alloc(deque, data);
  if (Z_MALLOC_IS_NULL(entry))
    return(RALEIGHSL_ERRNO_NO_MEMORY);

  /* Add only a single element to the transaction */
  if (transaction != NULL && txn_id != *current_txn_id) {
    raleighsl_errno_t errno;
    if ((errno = raleighsl_transaction_add(fs, transaction, object, current_txn_id))) {
      __deque_entry_free(deque, entry);
      return(errno);
    }
  }

  Z_LOG_TRACE("Push current_txn_id=%"PRIu64" txn_id=%"PRIu64, *current_txn_id = txn_id);

  /* Push entry to the pending deque */
  *current_txn_id = txn_id;
  z_dlink_add(pending_deque, &(entry->node));
  return(RALEIGHSL_ERRNO_NONE);
}

raleighsl_errno_t raleighsl_deque_pop (raleighsl_t *fs,
                                       raleighsl_transaction_t *transaction,
                                       raleighsl_object_t *object,
                                       int pop_front,
                                       z_bytes_ref_t *data)
{
  raleighsl_deque_t *deque = RALEIGHSL_DEQUE(object->membufs);
  z_dlink_node_t **removed_entry;
  z_dlink_node_t *pending_deque;
  z_dlink_node_t *node;
  uint64_t *current_txn_id;
  uint64_t txn_id;

  if (pop_front) {
    removed_entry  = &(deque->removed_front);
    pending_deque  = &(deque->pending_front);
    current_txn_id = &(deque->txn_id_front);
  } else {
    removed_entry  = &(deque->removed_back);
    pending_deque  = &(deque->pending_back);
    current_txn_id = &(deque->txn_id_back);
  }

  /* Verify that no other transaction is holding the operation-lock */
  txn_id = (transaction != NULL) ? raleighsl_txn_id(transaction) : 0;
  if (*current_txn_id > 0 && *current_txn_id != txn_id)
    return(RALEIGHSL_ERRNO_TXN_LOCKED_OPERATION);

  if (z_dlink_is_not_empty(pending_deque)) {
    struct deque_entry *entry;
    node = z_dlink_front(pending_deque);
    entry = z_dlink_entry(node, struct deque_entry, node);
    z_bytes_ref_acquire(data, &(entry->data));
    z_dlink_del(node);
    __deque_entry_free(deque, node);
    return(RALEIGHSL_ERRNO_NONE);
  }

  if (*removed_entry != &(deque->data) && z_dlink_is_not_empty(&(deque->data))) {
    struct deque_entry *entry;

    if (*removed_entry == NULL) {
      if (pop_front) {
        *removed_entry = z_dlink_front(&(deque->data));
      } else {
        *removed_entry = z_dlink_back(&(deque->data));
      }
    }

    entry = z_dlink_entry(*removed_entry, struct deque_entry, node);
    z_bytes_ref_acquire(data, &(entry->data));

    if (pop_front) {
      *removed_entry = (*removed_entry)->next;
    } else {
      *removed_entry = (*removed_entry)->prev;
    }

    *current_txn_id = txn_id;
    return(RALEIGHSL_ERRNO_NONE);
  }

  /* Remove from the other side */
  if (pop_front) {
    pending_deque  = &(deque->pending_back);
    current_txn_id = &(deque->txn_id_back);
  } else {
    pending_deque  = &(deque->pending_front);
    current_txn_id = &(deque->txn_id_front);
  }

  if (z_dlink_is_not_empty(pending_deque)) {
    struct deque_entry *entry;
    node = z_dlink_back(pending_deque);
    entry = z_dlink_entry(node, struct deque_entry, node);
    z_bytes_ref_acquire(data, &(entry->data));
    z_dlink_del(node);
    __deque_entry_free(deque, node);
    return(RALEIGHSL_ERRNO_NONE);
  }

  return(RALEIGHSL_ERRNO_DATA_NO_ITEMS);
}

/* ============================================================================
 *  PUBLIC Deque READ methods
 */

/* ============================================================================
 *  Deque Object Plugin
 */
static raleighsl_errno_t __object_create (raleighsl_t *fs,
                                          raleighsl_object_t *object)
{
  raleighsl_deque_t *deque;

  deque = z_memory_struct_alloc(z_global_memory(), raleighsl_deque_t);
  if (Z_MALLOC_IS_NULL(deque))
    return(RALEIGHSL_ERRNO_NO_MEMORY);

  z_dlink_init(&(deque->data));
  z_dlink_init(&(deque->pending_front));
  z_dlink_init(&(deque->pending_back));
  deque->removed_front = NULL;
  deque->removed_back = NULL;
  deque->txn_id_front = 0;
  deque->txn_id_back = 0;

  object->membufs = deque;
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __object_close (raleighsl_t *fs,
                                         raleighsl_object_t *object)
{
  raleighsl_deque_t *deque = RALEIGHSL_DEQUE(object->membufs);
  /* TODO: Clear */
  z_memory_struct_free(z_global_memory(), raleighsl_deque_t, deque);
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __object_commit (raleighsl_t *fs,
                                          raleighsl_object_t *object)
{
  raleighsl_deque_t *deque = RALEIGHSL_DEQUE(object->membufs);
  struct deque_entry *entry;
  z_dlink_node_t *node;

  if (deque->txn_id_front == 0) {
    /* Remove front */
    if (deque->removed_front != NULL) {
      node = z_dlink_front(&(deque->data));
      while (node != deque->removed_front) {
        entry = z_dlink_entry(node, struct deque_entry, node);
        z_dlink_del(node);
        __deque_entry_free(deque, entry);
        node = z_dlink_front(&(deque->data));
      }
      deque->removed_front = NULL;
    }

    /* Add front */
    while (z_dlink_is_not_empty(&(deque->pending_front))) {
      node = z_dlink_back(&(deque->pending_front));
      z_dlink_move(&(deque->data), node);
    }
  }

  if (deque->txn_id_back == 0) {
    /* Remove back */
    if (deque->removed_back != NULL) {
      node = z_dlink_back(&(deque->data));
      while (node != deque->removed_back) {
        z_dlink_del(node);
        entry = z_dlink_entry(node, struct deque_entry, node);
        __deque_entry_free(deque, entry);
        node = z_dlink_back(&(deque->data));
      }
      deque->removed_back = NULL;
    }

    /* Add back */
    while (z_dlink_is_not_empty(&(deque->pending_back))) {
      node = z_dlink_back(&(deque->pending_back));
      z_dlink_move_tail(&(deque->data), node);
    }
  }

  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __object_rollback (raleighsl_t *fs,
                                            raleighsl_object_t *object)
{
  raleighsl_deque_t *deque = RALEIGHSL_DEQUE(object->membufs);
  struct deque_entry *entry;

  if (deque->txn_id_front == 0) {
    z_dlink_for_each_safe_entry(&(deque->pending_front), entry, struct deque_entry, node, {
      __deque_entry_free(deque, entry);
    });
    z_dlink_init(&(deque->pending_front));
    deque->removed_front = NULL;
  }

  if (deque->txn_id_back == 0) {
    z_dlink_for_each_safe_entry(&(deque->pending_back), entry, struct deque_entry, node, {
      __deque_entry_free(deque, entry);
    });
    z_dlink_init(&(deque->pending_back));
    deque->removed_back = NULL;
  }

  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __object_apply (raleighsl_t *fs,
                                         raleighsl_object_t *object,
                                         void *mutation)
{
  uint64_t *current_txn_id = (uint64_t *)mutation;
  *current_txn_id = 0;
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __object_revert (raleighsl_t *fs,
                                          raleighsl_object_t *object,
                                          void *mutation)
{
  raleighsl_deque_t *deque = RALEIGHSL_DEQUE(object->membufs);
  uint64_t *current_txn_id = (uint64_t *)mutation;
  struct deque_entry *entry;

  if (current_txn_id == &(deque->txn_id_front)) {
    z_dlink_for_each_safe_entry(&(deque->pending_front), entry, struct deque_entry, node, {
      __deque_entry_free(deque, entry);
    });
    z_dlink_init(&(deque->pending_front));
    deque->removed_front = NULL;
    *current_txn_id = 0;
  } else if (current_txn_id == &(deque->txn_id_back)) {
    z_dlink_for_each_safe_entry(&(deque->pending_back), entry, struct deque_entry, node, {
      __deque_entry_free(deque, entry);
    });
    z_dlink_init(&(deque->pending_back));
    deque->removed_back = NULL;
    *current_txn_id = 0;
  }

  return(RALEIGHSL_ERRNO_NONE);
}

const raleighsl_object_plug_t raleighsl_object_deque = {
  .info = {
    .type = RALEIGHSL_PLUG_TYPE_OBJECT,
    .description = "Deque Object",
    .label       = "deque",
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
