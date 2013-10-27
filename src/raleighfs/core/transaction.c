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

#include <raleighfs/transaction.h>
#include <raleighfs/exec.h>

#include <zcl/global.h>
#include <zcl/ticket.h>
#include <zcl/debug.h>
#include <zcl/time.h>

#include "private.h"

struct raleighfs_txn_mgr {
  uint64_t next_txn_id;
  z_cache_t *cache;
  z_ticket_t lock;
};

struct txn_atom {
  struct txn_atom *next;
  void *mutation;
};

enum txn_sched_state {
  TXN_SCHED_ACQUIRE,
  TXN_SCHED_BARRIER,
  TXN_SCHED_LOCK,
  TXN_SCHED_WRITE,
  TXN_SCHED_COMMIT,
  TXN_SCHED_COMPLETE,
};

enum txn_commit_type {
  TXN_APPLY,
  TXN_REVERT,
};

struct txn_obj_group {
  struct txn_obj_group *next;
  raleighfs_object_t *object;
  struct txn_atom *atoms_head;
  struct txn_atom *atoms_tail;
};

/* ============================================================================
 *  PRIVATE TXN Atom
 */
static struct txn_atom *__txn_atom_alloc (void *mutation) {
  struct txn_atom *atom;

  atom = z_memory_struct_alloc(z_global_memory(), struct txn_atom);
  if (Z_MALLOC_IS_NULL(atom))
    return(NULL);

  atom->next = NULL;
  atom->mutation = mutation;
  return(atom);
}

static void __txn_atom_free (struct txn_atom *atom) {
  Z_ASSERT(atom->mutation == NULL, "the txn mutation must be freed by the owner");
  z_memory_struct_free(z_global_memory(), struct txn_atom, atom);
}

static struct txn_obj_group *__txn_object_group_alloc (raleighfs_object_t *object) {
  struct txn_obj_group *group;

  group = z_memory_struct_alloc(z_global_memory(), struct txn_obj_group);
  if (Z_MALLOC_IS_NULL(group))
    return(NULL);

  group->next = NULL;
  group->object = object;
  group->atoms_head = NULL;
  group->atoms_tail = NULL;
  return(group);
}

/* ============================================================================
 *  PRIVATE Object group methods
 */
static void __txn_object_group_free (struct txn_obj_group *group) {
  struct txn_atom *p;
  /* TODO: Free the atoms? */
  for (p = group->atoms_head; p != NULL; p = p->next) {
    __txn_atom_free(p);
  }

  z_memory_struct_free(z_global_memory(), struct txn_obj_group, group);
}

static struct txn_obj_group *__txn_object_group_add (raleighfs_transaction_t *transaction,
                                                     raleighfs_object_t *object)
{
  struct txn_obj_group *group = (struct txn_obj_group *)transaction->objects;
  struct txn_obj_group *prev;

  /* Try to lookup the group */
  for (prev = group; group != NULL; group = group->next) {
    if (group->object == object)
      return(group);
    prev = group;
  }

  /* Allocate new group */
  group = __txn_object_group_alloc(object);
  if (Z_MALLOC_IS_NULL(group))
    return(NULL);

  /* Add the group to the list */
  if (prev == NULL) {
    transaction->objects = group;
  } else {
    prev->next = group;
  }
  return(group);
}

/* ============================================================================
 *  PRIVATE Transaction methods
 */
raleighfs_transaction_t *raleighfs_transaction_alloc (raleighfs_t *fs, uint64_t txn_id) {
  raleighfs_transaction_t *txn;

  txn = z_memory_struct_alloc(z_global_memory(), raleighfs_transaction_t);
  if (Z_MALLOC_IS_NULL(txn))
    return(NULL);

  /* Initialize cache-entry attributes */
  z_cache_entry_init(Z_CACHE_ENTRY(txn), txn_id);

  /* Initialize Transactions's attributes */
  txn->objects = NULL;
  txn->mtime = z_time_millis();
  z_task_rwcsem_open(&(txn->rwcsem));
  txn->state = RALEIGHFS_TXN_WAIT_COMMIT;
  return(txn);
}

void raleighfs_transaction_free (raleighfs_t *fs, raleighfs_transaction_t *txn) {
  struct txn_obj_group *group = (struct txn_obj_group *)txn->objects;
  while (group != NULL) {
    struct txn_obj_group *group_next = group->next;
    __txn_object_group_free(group);
    group = group_next;
  }
  z_task_rwcsem_close(&(txn->rwcsem));
  z_memory_struct_free(z_global_memory(), raleighfs_transaction_t, txn);
}

/* ============================================================================
 *  RaleighFS Transaction Scheduler
 */
static int __txn_mgr_acquire_barrier (raleighfs_t *fs, raleighfs_transaction_t *txn) {
  struct txn_obj_group *group;
  int is_available = 1;

  z_ticket_acquire(&(fs->txn_mgr->lock));

  for (group = (struct txn_obj_group *)txn->objects; group != NULL; group = group->next) {
    if (group->object->pending_txn_id != 0) {
      is_available = 0;
      break;
    }
  }

  if (is_available) {
    for (group = (struct txn_obj_group *)txn->objects; group != NULL; group = group->next) {
      raleighfs_object_t *object = group->object;
      object->pending_txn_id = raleighfs_txn_id(txn);
      z_rwcsem_set_lock_flag(&(object->rwcsem.lock));
    }
  }

  z_ticket_release(&(fs->txn_mgr->lock));
  return(is_available);
}

static void __txn_mgr_release_locks (raleighfs_t *fs, raleighfs_transaction_t *txn) {
  struct txn_obj_group *group;
  z_ticket_acquire(&(fs->txn_mgr->lock));
  for (group = (struct txn_obj_group *)txn->objects; group != NULL; group = group->next) {
    raleighfs_object_t *object = group->object;
    Z_ASSERT(object->pending_txn_id == raleighfs_txn_id(txn),
             "unlocking the wrong transaction current=%u expected=%u oid=%u",
             object->pending_txn_id, raleighfs_txn_id(txn), raleighfs_oid(object));
    z_task_rwcsem_release(&(object->rwcsem), Z_RWCSEM_LOCK, NULL, 1);
    object->pending_txn_id = 0;
  }
  z_ticket_release(&(fs->txn_mgr->lock));
}

/* ============================================================================
 *  RaleighFS Transaction Scheduler
 */
#define __sched_task_notify_func_exec(fs, oid, errno, task)             \
  ((raleighfs_notify_func_t)((task)->args[0].ptr))                      \
    (fs, oid, errno, (task)->udata, ((task)->args[1].ptr),              \
      (task)->request, (task)->response)

static void __sched_txn_task_exec (z_task_t *task) {
  raleighfs_transaction_t *txn = RALEIGHFS_TRANSACTION(task->object.ptr);
  enum txn_commit_type commit_type = task->args[2].fd;
  raleighfs_errno_t errno = RALEIGHFS_ERRNO_NONE;
  raleighfs_t *fs = RALEIGHFS(task->context);
  int is_complete = 0;

  /* Acquire the transaction lock */
  if (task->state == TXN_SCHED_ACQUIRE) {
    Z_LOG_TRACE("Try acquire commit on TXN-ID %u", raleighfs_txn_id(txn));
    if (z_task_rwcsem_acquire(&(txn->rwcsem), Z_RWCSEM_COMMIT, task))
      return;

    task->state = TXN_SCHED_BARRIER;
  }

  /* Acquire the objects transaction lock */
  if (task->state == TXN_SCHED_BARRIER) {
    Z_LOG_TRACE("Try acquire object lock flags on TXN-ID %u", raleighfs_txn_id(txn));
    if (!__txn_mgr_acquire_barrier(fs, txn)) {
      /* TODO: Use a task sem? */
      z_global_add_pending_tasks(task);
      return;
    }
    task->state = TXN_SCHED_LOCK;
    task->args[3].ptr = txn->objects;
  }

  if (task->state == TXN_SCHED_LOCK) {
    struct txn_obj_group *group;

    Z_LOG_TRACE("Try acquire object locks on TXN-ID %u", raleighfs_txn_id(txn));
    group = (struct txn_obj_group *)task->args[3].ptr;
    while (group != NULL) {
      task->args[3].ptr = group;
      if (z_task_rwcsem_acquire(&(group->object->rwcsem), Z_RWCSEM_LOCK, task))
        return;

      group = group->next;
    }

    task->args[3].ptr = NULL;
    task->state = TXN_SCHED_WRITE;
  }

  if (task->state == TXN_SCHED_WRITE) {
    struct txn_obj_group *group;

    /* Revert instead of committing, if an error occurred */
    if (commit_type == TXN_APPLY && txn->state == RALEIGHFS_TXN_DONT_COMMIT) {
      Z_LOG_TRACE("TXN-ID %u COMMIT reverted to ROLLBACK", raleighfs_txn_id(txn));
      commit_type = TXN_REVERT;
    }

    /* Apply the txn */
    Z_LOG_TRACE("%s atoms on TXN-ID %u", (commit_type == TXN_APPLY ? "Apply" : "Revert"), raleighfs_txn_id(txn));
    for (group = (struct txn_obj_group *)txn->objects; group != NULL && !errno; group = group->next) {
      raleighfs_object_t *object = group->object;
      struct txn_atom *atom;

      atom = group->atoms_head;
      while (atom != NULL && !errno) {
        struct txn_atom *atom_next = atom->next;

        if (commit_type == TXN_APPLY) {
          Z_LOG_TRACE("Apply Txn-ID=%u Atom=%p on OID=%u",
                      raleighfs_txn_id(txn), atom, raleighfs_oid(object));
          errno = raleighfs_object_apply(fs, object, atom->mutation);
        } else {
          Z_LOG_TRACE("Revert Txn-ID=%u Atom=%p on OID=%u",
                      raleighfs_txn_id(txn), atom, raleighfs_oid(object));
          errno = raleighfs_object_revert(fs, object, atom->mutation);
        }
        /* TODO: How to handle commit error */

        /* Release the processed atom - the mutation should be freed by the object */
        atom->mutation = NULL;
        __txn_atom_free(atom);
        atom = atom_next;
        group->atoms_head = atom_next;
      }
    }

    /* Upgrade to commit */
    if (errno) {
      Z_LOG_TRACE("Rollback TXN-ID %u - %s", raleighfs_txn_id(txn), raleighfs_errno_string(errno));
      is_complete = 1;
      for (group = (struct txn_obj_group *)txn->objects; group != NULL; group = group->next) {
        /* TODO: Revert pending atoms */
        raleighfs_object_t *object = group->object;
        raleighfs_object_rollback(fs, object);
      }
    } else {
      Z_LOG_TRACE("Prepare Commit on TXN-ID %u", raleighfs_txn_id(txn));
      task->state = TXN_SCHED_COMMIT;
    }
  }

  /* Execute the transaction commit */
  if (task->state == TXN_SCHED_COMMIT) {
    struct txn_obj_group *group;

    Z_LOG_TRACE("Commit TXN-ID %u", raleighfs_txn_id(txn));
    for (group = (struct txn_obj_group *)txn->objects; group != NULL; group = group->next) {
      raleighfs_object_t *object = group->object;

      errno = raleighfs_object_commit(fs, object);
      /* TODO: How to handle commit error? */
    }

    is_complete = 1;
    task->state = TXN_SCHED_COMPLETE;
  }

  Z_ASSERT(is_complete == 1, "TXN not completed");
  if (!errno && commit_type == TXN_APPLY) {
    if (txn->state == RALEIGHFS_TXN_DONT_COMMIT) {
      errno = RALEIGHFS_ERRNO_TXN_ROLLEDBACK;
      txn->state = RALEIGHFS_TXN_ROLLEDBACK;
    } else {
      txn->state = RALEIGHFS_TXN_COMMITTED;
    }
  } else {
    txn->state = RALEIGHFS_TXN_ROLLEDBACK;
  }

  Z_LOG_TRACE("Completed %d TXN-ID %u - %s", is_complete, raleighfs_txn_id(txn), raleighfs_errno_string(errno));
  __txn_mgr_release_locks(fs, txn);
  z_task_rwcsem_release(&(txn->rwcsem), Z_RWCSEM_COMMIT, task, 1);
  __sched_task_notify_func_exec(fs, 0, errno, task);
  z_cache_release(fs->txn_mgr->cache, Z_CACHE_ENTRY(txn));
  z_task_free(task);
}

static int __txn_commit_task (raleighfs_t *fs,
                               uint64_t txn_id,
                              enum txn_commit_type commit_type,
                              raleighfs_notify_func_t notify_func,
                              void *req, void *resp,
                              void *udata, void *err_data)
{
  raleighfs_transaction_t *txn;
  z_task_t *task;

  task = z_task_alloc(__sched_txn_task_exec);
  if (Z_MALLOC_IS_NULL(task)) {
    return(-1);
  }

  txn = RALEIGHFS_TRANSACTION(z_cache_remove(fs->txn_mgr->cache, txn_id));
  if (txn == NULL) {
    z_task_free(task);
    notify_func(fs, 0, RALEIGHFS_ERRNO_TXN_NOT_FOUND, udata, err_data, req, resp);
    return(0);
  }

  z_rwcsem_set_commit_flag(&(txn->rwcsem.lock));
  task->state = TXN_SCHED_ACQUIRE;
  task->context = fs;
  task->object.ptr = txn;
  task->request = req;
  task->response = resp;
  task->udata = udata;
  task->args[0].ptr = notify_func;
  task->args[1].ptr = err_data;
  task->args[2].fd = commit_type;
  task->args[3].ptr = NULL; /* wlock */
  z_global_add_task(task);
  return(0);
}


/* ============================================================================
 *  PUBLIC Transaction WRITE methods
 */
static void __txn_cache_entry_free (void *fs, void *txn) {
  /* TODO: Rollback transaction */
  Z_LOG_WARN("TODO: Rollback transaction %u", raleighfs_txn_id(txn));
  raleighfs_transaction_free(RALEIGHFS(fs), RALEIGHFS_TRANSACTION(txn));
}

static int __txn_cache_entry_evict (void *udata,
                                    unsigned int size,
                                    z_cache_entry_t *entry)
{
  raleighfs_transaction_t *txn = RALEIGHFS_TRANSACTION(entry);
  uint64_t txn_time = z_time_millis() - txn->mtime;

  if (txn_time > (60 * 1000)) {
    Z_LOG_TRACE("Transaction is active since %.2fsec", txn_time / 1000.0f);
    /* TODO: Evict by expire time */
  }
  return(0);
}

int raleighfs_txn_mgr_alloc (raleighfs_t *fs) {
  raleighfs_txn_mgr_t *txn_mgr;
  z_cache_t *cache;

  cache = z_cache_alloc(Z_CACHE_LRU, 100000,
                        __txn_cache_entry_free,
                        __txn_cache_entry_evict, fs);
  if (Z_MALLOC_IS_NULL(cache))
    return(1);

  txn_mgr = z_memory_struct_alloc(z_global_memory(), raleighfs_txn_mgr_t);
  if (Z_MALLOC_IS_NULL(txn_mgr)) {
    z_cache_free(cache);
    return(2);
  }

  txn_mgr->next_txn_id = 1;
  txn_mgr->cache = cache;
  z_ticket_init(&(txn_mgr->lock));

  fs->txn_mgr = txn_mgr;
  return(0);
}

void raleighfs_txn_mgr_free (raleighfs_t *fs) {
  raleighfs_txn_mgr_t *txn_mgr = fs->txn_mgr;
  z_cache_free(txn_mgr->cache);
  z_memory_struct_free(z_global_memory(), raleighfs_txn_mgr_t, txn_mgr);
}

raleighfs_errno_t raleighfs_transaction_add (raleighfs_t *fs,
                                             raleighfs_transaction_t *transaction,
                                             raleighfs_object_t *object,
                                             void *mutation)
{
  struct txn_obj_group *group;
  struct txn_atom *atom;

  /* lookup/allocate object group */
  group = __txn_object_group_add(transaction, object);
  if (Z_MALLOC_IS_NULL(group))
    return(RALEIGHFS_ERRNO_NO_MEMORY);

  /* Allocate new txn-atom */
  atom = __txn_atom_alloc(mutation);
  if (Z_MALLOC_IS_NULL(atom))
    return(RALEIGHFS_ERRNO_NO_MEMORY);

  if (group->atoms_tail != NULL) {
    group->atoms_tail->next = atom;
  } else {
    group->atoms_head = atom;
  }
  group->atoms_tail = atom;

  transaction->mtime = z_time_millis();

  Z_LOG_TRACE("Add Txn-ID=%u Atom=%p", raleighfs_txn_id(transaction), atom);
  return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_errno_t raleighfs_transaction_create (raleighfs_t *fs,
                                                uint64_t *txn_id)
{
  raleighfs_transaction_t *txn;

  *txn_id = z_atomic_inc(&(fs->txn_mgr->next_txn_id));

  txn = raleighfs_transaction_alloc(fs, *txn_id);
  if (Z_MALLOC_IS_NULL(txn))
    return(RALEIGHFS_ERRNO_NO_MEMORY);

  Z_LOG_TRACE("Create new Transaction %u", *txn_id);
  txn = RALEIGHFS_TRANSACTION(z_cache_try_insert(fs->txn_mgr->cache,
                                                 Z_CACHE_ENTRY(txn)));
  Z_ASSERT(txn == NULL, "Another transaction with same id present %p", txn);

  return(RALEIGHFS_ERRNO_NONE);
}

int raleighfs_exec_txn_commit (raleighfs_t *fs,
                               uint64_t txn_id,
                               raleighfs_notify_func_t notify_func,
                               void *req, void *resp,
                               void *udata, void *err_data)
{
  return(__txn_commit_task(fs, txn_id, TXN_APPLY, notify_func, req, resp, udata, err_data));
}

int raleighfs_exec_txn_rollback (raleighfs_t *fs,
                                 uint64_t txn_id,
                                 raleighfs_notify_func_t notify_func,
                                 void *req, void *resp,
                                 void *udata, void *err_data)
{
  return(__txn_commit_task(fs, txn_id, TXN_REVERT, notify_func, req, resp, udata, err_data));
}

/* ============================================================================
 *  PUBLIC Transaction READ methods
 */
raleighfs_errno_t raleighfs_transaction_acquire (raleighfs_t *fs,
                                                 uint64_t txn_id,
                                                 raleighfs_transaction_t **txn)
{
  *txn = NULL;
  if (txn_id > 0) {
    raleighfs_transaction_t *txn_ctx;

    Z_LOG_TRACE("Acquire Txn-ID %u", txn_id);
    txn_ctx = RALEIGHFS_TRANSACTION(z_cache_lookup(fs->txn_mgr->cache, txn_id));
    if (txn_ctx == NULL)
      return(RALEIGHFS_ERRNO_TXN_NOT_FOUND);

    if (!z_rwcsem_try_acquire_read(&(txn_ctx->rwcsem.lock)))
      return(RALEIGHFS_ERRNO_TXN_CLOSED);

    *txn = txn_ctx;
  }
  return(RALEIGHFS_ERRNO_NONE);
}

void raleighfs_transaction_release (raleighfs_t *fs,
                                    raleighfs_transaction_t *txn)
{
  if (txn == NULL)
    return;

  Z_LOG_TRACE("Release Txn-ID %u", raleighfs_txn_id(txn));
  z_rwcsem_release_read(&(txn->rwcsem.lock));
  z_cache_release(fs->txn_mgr->cache, Z_CACHE_ENTRY(txn));
}
