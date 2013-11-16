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

#include <zcl/locking.h>
#include <zcl/global.h>
#include <zcl/debug.h>

#include "private.h"

/* ============================================================================
 *  PUBLIC Object methods
 */
raleighsl_errno_t raleighsl_object_create (raleighsl_t *fs,
                                           const raleighsl_object_plug_t *plug,
                                           uint64_t oid)
{
  raleighsl_object_t *object;
  raleighsl_errno_t errno;

  /* Add Object to cache */
  object = raleighsl_obj_cache_get(fs, oid);
  if (Z_MALLOC_IS_NULL(object))
    return(RALEIGHSL_ERRNO_NO_MEMORY);

  /* Assign plug to object */
  object->plug = plug;

  /* Object create */
  if ((errno = __object_call_required(fs, object, create))) {
    return(errno);
  }

  raleighsl_obj_cache_release(fs, object);

  return(RALEIGHSL_ERRNO_NONE);
}

raleighsl_errno_t raleighsl_object_open (raleighsl_t *fs,
                                         raleighsl_object_t *object)
{
  raleighsl_errno_t errno;

  /* TODO */
  if (object->plug == NULL) {
    Z_LOG_WARN("TODO: Implement open, missing plugin for object %"PRIu64, raleighsl_oid(object));
    return(RALEIGHSL_ERRNO_PLUGIN_NOT_LOADED);
  }

  if (raleighsl_object_is_open(fs, object))
    return(RALEIGHSL_ERRNO_NONE);

  /* Object open */
  if ((errno = __object_call_required(fs, object, open))) {
    return(errno);
  }

  return(RALEIGHSL_ERRNO_NONE);
}

raleighsl_errno_t raleighsl_object_close (raleighsl_t *fs,
                                          raleighsl_object_t *object)
{
  return(__object_call_required(fs, object, close));
}

raleighsl_errno_t raleighsl_object_sync (raleighsl_t *fs,
                                         raleighsl_object_t *object)
{
  return(__object_call_required(fs, object, sync));
}

/* ============================================================================
 *  RaleighSL Object Scheduler
 */
enum object_sched_state {
  OBJECT_SCHED_OPEN   = 0,
  OBJECT_SCHED_READ   = 1,
  OBJECT_SCHED_WRITE  = 2,
  OBJECT_SCHED_COMMIT = 3,
};

static z_rwcsem_op_t __sched_state_rwc_op[] = {
  [OBJECT_SCHED_OPEN]   = Z_RWCSEM_WRITE,
  [OBJECT_SCHED_READ]   = Z_RWCSEM_READ,
  [OBJECT_SCHED_WRITE]  = Z_RWCSEM_WRITE,
  [OBJECT_SCHED_COMMIT] = Z_RWCSEM_COMMIT,
};

#define __sched_task_read_func_exec(fs, txn, object, task)                \
  ((raleighsl_read_func_t)((task)->args[2].ptr))                          \
    (fs, txn, object, (task)->udata)

#define __sched_task_write_func_exec(fs, txn, object, task)               \
  ((raleighsl_write_func_t)((task)->args[2].ptr))                         \
    (fs, txn, object, (task)->udata)

#define __sched_task_notify_func_exec(fs, oid, errno, task)               \
  ((raleighsl_notify_func_t)((task)->args[0].ptr))                        \
    (fs, oid, errno, (task)->udata, ((task)->args[1].ptr))

static void __sched_object_task_exec (z_task_t *task) {
  raleighsl_t *fs = RALEIGHSL(task->context);
  raleighsl_transaction_t *txn;
  raleighsl_object_t *object;
  raleighsl_errno_t errno;
  z_rwcsem_op_t op_type;
  int keep_running = 0;
  int is_complete = 1;

  txn = RALEIGHSL_TRANSACTION(task->args[3].ptr);
  if (task->state == OBJECT_SCHED_OPEN) {
    object = raleighsl_obj_cache_get(fs, task->object.u64);
    if (raleighsl_object_is_open(fs, object)) {
      task->object.ptr = object;
      task->state = task->flags;
    }
  } else {
    object = RALEIGHSL_OBJECT(task->object.ptr);
  }

  /* let a commit finish before a transaction will begin */
  if (task->state != OBJECT_SCHED_COMMIT && object->pending_txn_id != 0) {
    Z_LOG_TRACE("Stuck with pending TXN-ID %"PRIu64, object->pending_txn_id);
    /* TODO: Use a task sem? */
    z_global_add_pending_tasks(task);
    return;
  }

  op_type = __sched_state_rwc_op[task->state];
  if (z_task_rwcsem_acquire(&(object->rwcsem), op_type, task)) {
    return;
  }

  /* Execute the task */
  do {
    keep_running = 0;
    is_complete = 1;

    op_type = __sched_state_rwc_op[task->state];
    switch (task->state) {
      case OBJECT_SCHED_OPEN:
        task->object.ptr = object;
        errno = raleighsl_object_open(fs, object);
        if (errno) {
          if (object->plug != NULL) {
            raleighsl_object_rollback(fs, object);
          }
        } else {
          is_complete = 0;
          task->state = task->flags;
          keep_running = z_rwcsem_try_switch(&(object->rwcsem.lock), op_type, __sched_state_rwc_op[task->state]);
        }
        break;
      case OBJECT_SCHED_READ:
        errno = __sched_task_read_func_exec(fs, txn, object, task);
        if (errno == RALEIGHSL_ERRNO_SCHED_YIELD) {
          is_complete = 0;
        }
        break;
      case OBJECT_SCHED_WRITE:
        errno = __sched_task_write_func_exec(fs, txn, object, task);
        if (errno) {
          if (txn != NULL) txn->state = RALEIGHSL_TXN_DONT_COMMIT;
          raleighsl_object_rollback(fs, object);
        } else {
          /* TODO: No need for commit if txn != NULL */
          z_rwcsem_set_commit_flag(&(object->rwcsem.lock));
          is_complete = 0;
          task->state = OBJECT_SCHED_COMMIT;
          keep_running = z_rwcsem_try_switch(&(object->rwcsem.lock), op_type, __sched_state_rwc_op[task->state]);
        }
        break;
      case OBJECT_SCHED_COMMIT:
        errno = raleighsl_object_commit(fs, object);
        if (Z_UNLIKELY(errno)) {
          raleighsl_object_rollback(fs, object);
        }
        break;
    }
  } while (keep_running);
  z_task_rwcsem_release(&(object->rwcsem), op_type, task, is_complete);

  if (is_complete) {
    __sched_task_notify_func_exec(fs, raleighsl_oid(object), errno, task);
    raleighsl_obj_cache_release(fs, object);
    raleighsl_transaction_release(fs, txn);
    z_task_free(task);
  }
}

/* ============================================================================
 *  PUBLIC Sched methods
 */
int raleighsl_exec_read (raleighsl_t *fs,
                         uint64_t txn_id, uint64_t oid,
                         raleighsl_read_func_t read_func,
                         raleighsl_notify_func_t notify_func,
                         void *udata, void *err_data)
{
  raleighsl_transaction_t *txn;
  raleighsl_errno_t errno;
  z_task_t *task;

  if ((errno = raleighsl_transaction_acquire(fs, txn_id, &txn))) {
    notify_func(fs, 0, errno, udata, err_data);
    return(0);
  }

  task = z_task_alloc(__sched_object_task_exec);
  if (Z_MALLOC_IS_NULL(task)) {
    raleighsl_transaction_release(fs, txn);
    return(-1);
  }

  task->state = OBJECT_SCHED_OPEN;
  task->flags = OBJECT_SCHED_READ;
  task->context = fs;
  task->object.u64 = oid;
  task->udata = udata;
  task->args[0].ptr = notify_func;
  task->args[1].ptr = err_data;
  task->args[2].ptr = read_func;
  task->args[3].ptr = txn;

  z_global_add_task(task);
  return(0);
}

int raleighsl_exec_write (raleighsl_t *fs,
                          uint64_t txn_id, uint64_t oid,
                          raleighsl_write_func_t write_func,
                          raleighsl_notify_func_t notify_func,
                          void *udata, void *err_data)
{
  raleighsl_transaction_t *txn;
  raleighsl_errno_t errno;
  z_task_t *task;

  if ((errno = raleighsl_transaction_acquire(fs, txn_id, &txn))) {
    notify_func(fs, 0, errno, udata, err_data);
    return(0);
  }

  task = z_task_alloc(__sched_object_task_exec);
  if (Z_MALLOC_IS_NULL(task)) {
    raleighsl_transaction_release(fs, txn);
    return(-1);
  }

  task->state = OBJECT_SCHED_OPEN;
  task->flags = OBJECT_SCHED_WRITE;
  task->context = fs;
  task->object.u64 = oid;
  task->udata = udata;
  task->args[0].ptr = notify_func;
  task->args[1].ptr = err_data;
  task->args[2].ptr = write_func;
  task->args[3].ptr = txn;

  z_global_add_task(task);
  return(0);
}
