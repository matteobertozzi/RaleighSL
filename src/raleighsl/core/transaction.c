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

#include <raleighsl/transaction.h>
#include <raleighsl/exec.h>

#include <zcl/global.h>
#include <zcl/ticket.h>
#include <zcl/debug.h>
#include <zcl/time.h>

#include "private.h"

struct raleighsl_txn_mgr {
  uint64_t next_txn_id;
  z_cache_t *cache;
  z_ticket_t lock;
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
  raleighsl_object_t *object;
  raleighsl_txn_atom_t *atoms_head;
  raleighsl_txn_atom_t *atoms_tail;
};

/* ============================================================================
 *  PRIVATE Transaction macros
 */
#define __txn_from_cache_entry(entry)                         \
  z_container_of(entry, raleighsl_transaction_t, cache_entry)

/* ============================================================================
 *  PRIVATE Object group methods
 */
static struct txn_obj_group *__txn_object_group_alloc (raleighsl_object_t *object) {
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

static void __txn_object_group_free (struct txn_obj_group *group) {
  /* TODO: Free the atoms?
  for (p = group->atoms_head; p != NULL; p = p->next) {
    TODO: Free the atom?
  }
  */
  z_memory_struct_free(z_global_memory(), struct txn_obj_group, group);
}

static struct txn_obj_group *__txn_object_group (raleighsl_transaction_t *transaction,
                                                 raleighsl_object_t *object)
{
  struct txn_obj_group *group = (struct txn_obj_group *)transaction->objects;
  while (group != NULL) {
    if (group->object == object)
      return(group);
    group = group->next;
  }
  return(NULL);
}

static struct txn_obj_group *__txn_object_group_add (raleighsl_transaction_t *transaction,
                                                     raleighsl_object_t *object)
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
raleighsl_transaction_t *raleighsl_transaction_alloc (raleighsl_t *fs, uint64_t txn_id) {
  raleighsl_transaction_t *txn;

  txn = z_memory_struct_alloc(z_global_memory(), raleighsl_transaction_t);
  if (Z_MALLOC_IS_NULL(txn))
    return(NULL);

  /* Initialize cache-entry attributes */
  z_cache_entry_init(&(txn->cache_entry), txn_id);

  /* Initialize Transactions's attributes */
  txn->objects = NULL;
  txn->mtime = z_time_micros();
  z_task_rwcsem_open(&(txn->rwcsem));
  txn->state = RALEIGHSL_TXN_WAIT_COMMIT;
  z_ticket_init(&(txn->lock));
  return(txn);
}

void raleighsl_transaction_free (raleighsl_t *fs, raleighsl_transaction_t *txn) {
  struct txn_obj_group *group = (struct txn_obj_group *)txn->objects;
  while (group != NULL) {
    struct txn_obj_group *group_next = group->next;
    __txn_object_group_free(group);
    group = group_next;
  }
  z_task_rwcsem_close(&(txn->rwcsem));
  z_memory_struct_free(z_global_memory(), raleighsl_transaction_t, txn);
}

/* ============================================================================
 *  RaleighSL Transaction Scheduler
 */
static int __txn_mgr_acquire_barrier (raleighsl_t *fs, raleighsl_transaction_t *txn) {
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
      raleighsl_object_t *object = group->object;
      object->pending_txn_id = raleighsl_txn_id(txn);
      z_rwcsem_set_lock_flag(&(object->rwcsem.lock));
    }
  }

  z_ticket_release(&(fs->txn_mgr->lock));
  return(is_available);
}

static void __txn_mgr_release_locks (raleighsl_t *fs, raleighsl_transaction_t *txn) {
  struct txn_obj_group *group;
  z_ticket_acquire(&(fs->txn_mgr->lock));
  for (group = (struct txn_obj_group *)txn->objects; group != NULL; group = group->next) {
    raleighsl_object_t *object = group->object;
    Z_ASSERT(object->pending_txn_id == raleighsl_txn_id(txn),
             "unlocking the wrong transaction current=%"PRIu64" expected=%"PRIu64" oid=%"PRIu64"",
             object->pending_txn_id, raleighsl_txn_id(txn), raleighsl_oid(object));
    z_task_rwcsem_release(&(object->rwcsem), Z_RWCSEM_LOCK, NULL, 1);
    object->pending_txn_id = 0;
  }
  z_ticket_release(&(fs->txn_mgr->lock));
}

/* ============================================================================
 *  RaleighSL Transaction Scheduler
 */
#define __sched_task_notify_func_exec(fs, oid, errno, task)             \
  ((raleighsl_notify_func_t)((task)->args[0].ptr))                      \
    (fs, oid, errno, (task)->udata, ((task)->args[1].ptr))

static void __sched_txn_task_exec (z_task_t *task) {
  raleighsl_transaction_t *txn = RALEIGHSL_TRANSACTION(task->object.ptr);
  enum txn_commit_type commit_type = task->args[2].fd;
  raleighsl_errno_t errno = RALEIGHSL_ERRNO_NONE;
  raleighsl_t *fs = RALEIGHSL(task->context);
  int is_complete = 0;

  /* Acquire the transaction lock */
  if (task->state == TXN_SCHED_ACQUIRE) {
    Z_LOG_TRACE("Try acquire commit on TXN-ID %"PRIu64, raleighsl_txn_id(txn));
    if (z_task_rwcsem_acquire(&(txn->rwcsem), Z_RWCSEM_COMMIT, task))
      return;

    task->state = TXN_SCHED_BARRIER;
  }

  /* Acquire the objects transaction lock */
  if (task->state == TXN_SCHED_BARRIER) {
    Z_LOG_TRACE("Try acquire object lock flags on TXN-ID %"PRIu64, raleighsl_txn_id(txn));
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

    Z_LOG_TRACE("Try acquire object locks on TXN-ID %"PRIu64, raleighsl_txn_id(txn));
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
    if (commit_type == TXN_APPLY && txn->state == RALEIGHSL_TXN_DONT_COMMIT) {
      Z_LOG_TRACE("TXN-ID %"PRIu64" COMMIT reverted to ROLLBACK", raleighsl_txn_id(txn));
      commit_type = TXN_REVERT;
    }

    /* Apply the txn */
    Z_LOG_TRACE("%s atoms on TXN-ID %"PRIu64, (commit_type == TXN_APPLY ? "Apply" : "Revert"), raleighsl_txn_id(txn));
    for (group = (struct txn_obj_group *)txn->objects; group != NULL && !errno; group = group->next) {
      raleighsl_object_t *object = group->object;
      raleighsl_txn_atom_t *atom;

      atom = group->atoms_head;
      while (atom != NULL && !errno) {
        raleighsl_txn_atom_t *atom_next = atom->next;

        if (commit_type == TXN_APPLY) {
          Z_LOG_TRACE("Apply Txn-ID=%"PRIu64" Atom=%p on OID=%"PRIu64"",
                      raleighsl_txn_id(txn), atom, raleighsl_oid(object));
          raleighsl_object_apply(fs, object, atom);
        } else {
          Z_LOG_TRACE("Revert Txn-ID=%"PRIu64" Atom=%p on OID=%"PRIu64"",
                      raleighsl_txn_id(txn), atom, raleighsl_oid(object));
          raleighsl_object_revert(fs, object, atom);
        }
        /* TODO: How to handle commit error */

        /* Release the processed atom - the mutation should be freed by the object */
        atom = atom_next;
        group->atoms_head = atom_next;
      }
    }

    /* Upgrade to commit */
    if (errno) {
      Z_LOG_TRACE("Rollback TXN-ID %"PRIu64" - %s", raleighsl_txn_id(txn), raleighsl_errno_string(errno));
      is_complete = 1;
    } else {
      Z_LOG_TRACE("Prepare Commit on TXN-ID %"PRIu64, raleighsl_txn_id(txn));
      task->state = TXN_SCHED_COMMIT;
    }
  }

  /* Execute the transaction commit */
  if (task->state == TXN_SCHED_COMMIT) {
    struct txn_obj_group *group;

    Z_LOG_TRACE("Commit TXN-ID %"PRIu64, raleighsl_txn_id(txn));
    for (group = (struct txn_obj_group *)txn->objects; group != NULL; group = group->next) {
      raleighsl_object_t *object = group->object;

      errno = raleighsl_object_commit(fs, object);
      /* TODO: How to handle commit error? */
    }

    is_complete = 1;
    task->state = TXN_SCHED_COMPLETE;
  }

  Z_ASSERT(is_complete == 1, "TXN not completed");
  if (!errno && commit_type == TXN_APPLY) {
    if (txn->state == RALEIGHSL_TXN_DONT_COMMIT) {
      errno = RALEIGHSL_ERRNO_TXN_ROLLEDBACK;
      txn->state = RALEIGHSL_TXN_ROLLEDBACK;
    } else {
      txn->state = RALEIGHSL_TXN_COMMITTED;
    }
  } else {
    txn->state = RALEIGHSL_TXN_ROLLEDBACK;
  }

  Z_LOG_TRACE("Completed %d TXN-ID %"PRIu64" - %s", is_complete, raleighsl_txn_id(txn), raleighsl_errno_string(errno));
  __txn_mgr_release_locks(fs, txn);
  z_task_rwcsem_release(&(txn->rwcsem), Z_RWCSEM_COMMIT, task, is_complete);
  __sched_task_notify_func_exec(fs, 0, errno, task);
  z_cache_release(fs->txn_mgr->cache, &(txn->cache_entry));
  z_task_free(task);
}

static int __txn_commit_task (raleighsl_t *fs,
                              uint64_t txn_id,
                              enum txn_commit_type commit_type,
                              raleighsl_notify_func_t notify_func,
                              void *udata, void *err_data)
{
  raleighsl_transaction_t *txn;
  z_cache_entry_t *entry;
  z_task_t *task;

  task = z_task_alloc(__sched_txn_task_exec);
  if (Z_MALLOC_IS_NULL(task)) {
    return(-1);
  }

  entry = z_cache_remove(fs->txn_mgr->cache, txn_id);
  if (entry == NULL) {
    z_task_free(task);
    notify_func(fs, 0, RALEIGHSL_ERRNO_TXN_NOT_FOUND, udata, err_data);
    return(0);
  }

  txn = __txn_from_cache_entry(entry);
  z_rwcsem_set_commit_flag(&(txn->rwcsem.lock));
  task->state = TXN_SCHED_ACQUIRE;
  task->context = fs;
  task->object.ptr = txn;
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
static void __txn_cache_entry_free (void *fs, void *entry) {
  raleighsl_transaction_t *txn = __txn_from_cache_entry(entry);

  /* TODO: Rollback transaction */
  Z_LOG_WARN("TODO: Rollback transaction %"PRIu64, raleighsl_txn_id(txn));
  raleighsl_transaction_free(RALEIGHSL(fs), txn);
}

static int __txn_cache_entry_evict (void *udata,
                                    unsigned int size,
                                    z_cache_entry_t *entry)
{
  raleighsl_transaction_t *txn = __txn_from_cache_entry(entry);
  uint64_t txn_time = z_time_micros() - txn->mtime;

  if (txn_time > Z_TIME_SEC(60)) {
    Z_LOG_TRACE("Transaction is active since %.2fsec", txn_time / 1000000.0f);
    /* TODO: Evict by expire time */
  }
  return(0);
}

int raleighsl_txn_mgr_alloc (raleighsl_t *fs) {
  raleighsl_txn_mgr_t *txn_mgr;
  z_cache_t *cache;

  cache = z_cache_alloc(Z_CACHE_LRU, 10000,
                        __txn_cache_entry_free,
                        __txn_cache_entry_evict, fs);
  if (Z_MALLOC_IS_NULL(cache))
    return(1);

  txn_mgr = z_memory_struct_alloc(z_global_memory(), raleighsl_txn_mgr_t);
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

void raleighsl_txn_mgr_free (raleighsl_t *fs) {
  raleighsl_txn_mgr_t *txn_mgr = fs->txn_mgr;
  z_cache_free(txn_mgr->cache);
  z_memory_struct_free(z_global_memory(), raleighsl_txn_mgr_t, txn_mgr);
}

raleighsl_errno_t raleighsl_transaction_add (raleighsl_t *fs,
                                             raleighsl_transaction_t *transaction,
                                             raleighsl_object_t *object,
                                             raleighsl_txn_atom_t *atom)
{
  struct txn_obj_group *group;

  Z_ASSERT(atom != NULL, "Expected a NOT NULL atom");

  z_ticket_acquire(&(transaction->lock));

  /* lookup/allocate object group */
  group = __txn_object_group_add(transaction, object);
  if (Z_MALLOC_IS_NULL(group)) {
    z_ticket_release(&(transaction->lock));
    return(RALEIGHSL_ERRNO_NO_MEMORY);
  }

  atom->next = NULL;
  if (group->atoms_tail != NULL) {
    group->atoms_tail->next = atom;
  } else {
    group->atoms_head = atom;
  }
  group->atoms_tail = atom;

  transaction->mtime = z_time_micros();

  z_ticket_release(&(transaction->lock));

  Z_LOG_TRACE("Add Txn-ID=%"PRIu64" Atom=%p", raleighsl_txn_id(transaction), atom);
  return(RALEIGHSL_ERRNO_NONE);
}

void raleighsl_transaction_replace (raleighsl_t *fs,
                                    raleighsl_transaction_t *transaction,
                                    raleighsl_object_t *object,
                                    raleighsl_txn_atom_t *atom,
                                    raleighsl_txn_atom_t *new_atom)
{
  struct txn_obj_group *group;
  raleighsl_txn_atom_t *prev;
  raleighsl_txn_atom_t *p;

  Z_ASSERT(atom != NULL, "Expected a NOT NULL atom");

  z_ticket_acquire(&(transaction->lock));

  group = __txn_object_group(transaction, object);
  Z_ASSERT(atom != NULL, "Expected a group for the atom");

  for (p = prev = group->atoms_head; p != NULL; p = p->next) {
    if (p == atom) {
      if (new_atom == NULL) {
        prev->next = atom->next;
      } else {
        new_atom->next = p->next;
        prev->next = new_atom;
      }
      break;
    }
    prev = p;
  }
  z_ticket_release(&(transaction->lock));

  Z_ASSERT(prev->next == ((new_atom == NULL) ? atom->next : new_atom), "txn-atom not found");
  Z_LOG_TRACE("Replaced Txn-ID=%"PRIu64" Atom=%p with %p",
              raleighsl_txn_id(transaction), atom, new_atom);
}

void raleighsl_transaction_remove (raleighsl_t *fs,
                                   raleighsl_transaction_t *transaction,
                                   raleighsl_object_t *object,
                                   raleighsl_txn_atom_t *atom)
{
  raleighsl_transaction_replace(fs, transaction, object, atom, NULL);
}

raleighsl_errno_t raleighsl_transaction_create (raleighsl_t *fs,
                                                uint64_t *txn_id)
{
  raleighsl_transaction_t *txn;
  z_cache_entry_t *entry;

  *txn_id = z_atomic_inc(&(fs->txn_mgr->next_txn_id));

  txn = raleighsl_transaction_alloc(fs, *txn_id);
  if (Z_MALLOC_IS_NULL(txn))
    return(RALEIGHSL_ERRNO_NO_MEMORY);

  Z_LOG_TRACE("Create new Transaction %"PRIu64, *txn_id);
  entry = z_cache_try_insert(fs->txn_mgr->cache, &(txn->cache_entry));
  Z_ASSERT(entry == NULL, "Another transaction with same id present %"PRIu64,
                          raleighsl_txn_id(txn));

  return(RALEIGHSL_ERRNO_NONE);
}

int raleighsl_exec_txn_commit (raleighsl_t *fs,
                               uint64_t txn_id,
                               raleighsl_notify_func_t notify_func,
                               void *udata, void *err_data)
{
  return(__txn_commit_task(fs, txn_id, TXN_APPLY, notify_func, udata, err_data));
}

int raleighsl_exec_txn_rollback (raleighsl_t *fs,
                                 uint64_t txn_id,
                                 raleighsl_notify_func_t notify_func,
                                 void *udata, void *err_data)
{
  return(__txn_commit_task(fs, txn_id, TXN_REVERT, notify_func, udata, err_data));
}

/* ============================================================================
 *  PUBLIC Transaction READ methods
 */
raleighsl_errno_t raleighsl_transaction_acquire (raleighsl_t *fs,
                                                 uint64_t txn_id,
                                                 raleighsl_transaction_t **txn)
{
  *txn = NULL;
  if (txn_id > 0) {
    raleighsl_transaction_t *txn_ctx;
    z_cache_entry_t *entry;

    Z_LOG_TRACE("Acquire Txn-ID %"PRIu64, txn_id);
    entry = z_cache_lookup(fs->txn_mgr->cache, txn_id);
    if (entry == NULL)
      return(RALEIGHSL_ERRNO_TXN_NOT_FOUND);

    txn_ctx = __txn_from_cache_entry(entry);
    if (!z_rwcsem_try_acquire_read(&(txn_ctx->rwcsem.lock)))
      return(RALEIGHSL_ERRNO_TXN_CLOSED);

    *txn = txn_ctx;
  }
  return(RALEIGHSL_ERRNO_NONE);
}

void raleighsl_transaction_release (raleighsl_t *fs,
                                    raleighsl_transaction_t *txn)
{
  if (txn == NULL)
    return;

  Z_LOG_TRACE("Release Txn-ID %"PRIu64, raleighsl_txn_id(txn));
  z_rwcsem_release_read(&(txn->rwcsem.lock));
  z_cache_release(fs->txn_mgr->cache, &(txn->cache_entry));
}
