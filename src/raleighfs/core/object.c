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

#include <zcl/locking.h>
#include <zcl/global.h>
#include <zcl/debug.h>

#include "private.h"

/* ============================================================================
 *  PUBLIC Object methods
 */
raleighfs_errno_t raleighfs_object_create (raleighfs_t *fs,
                                           const raleighfs_object_plug_t *plug,
                                           uint64_t oid)
{
  raleighfs_object_t *object;
  raleighfs_errno_t errno;

  /* Add Object to cache */
  object = raleighfs_obj_cache_get(fs, oid);
  if (Z_MALLOC_IS_NULL(object))
    return(RALEIGHFS_ERRNO_NO_MEMORY);

  /* Assign plug to object */
  object->plug = plug;

  /* Object create */
  if ((errno = __object_call_required(fs, object, create))) {
    return(errno);
  }

  raleighfs_obj_cache_release(fs, object);

  return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_errno_t raleighfs_object_open (raleighfs_t *fs,
                                         raleighfs_object_t *object)
{
  raleighfs_errno_t errno;

  if (raleighfs_object_is_open(fs, object))
    return(RALEIGHFS_ERRNO_NONE);

  /* Object open */
  if ((errno = __object_call_required(fs, object, open))) {
    return(errno);
  }

  return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_errno_t raleighfs_object_close (raleighfs_t *fs,
                                          raleighfs_object_t *object)
{
  return(__object_call_required(fs, object, close));
}

raleighfs_errno_t raleighfs_object_sync (raleighfs_t *fs,
                                         raleighfs_object_t *object)
{
  return(__object_call_required(fs, object, sync));
}

/* ============================================================================
 *  RaleighFS Object Scheduler
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
  ((raleighfs_read_func_t)((task)->args[2].ptr))                          \
    (fs, txn, object, (task)->udata, (task)->request, (task)->response)

#define __sched_task_write_func_exec(fs, txn, object, task)               \
  ((raleighfs_write_func_t)((task)->args[2].ptr))                         \
    (fs, txn, object, (task)->udata, (task)->request, (task)->response)

#define __sched_task_notify_func_exec(fs, oid, errno, task)               \
  ((raleighfs_notify_func_t)((task)->args[0].ptr))                        \
    (fs, oid, errno, (task)->udata, ((task)->args[1].ptr),                \
      (task)->request, (task)->response)

static void __sched_object_task_exec (z_task_t *task) {
  raleighfs_t *fs = RALEIGHFS(task->context);
  z_rwcsem_op_t operation_type;
  raleighfs_transaction_t *txn;
  raleighfs_object_t *object;
  raleighfs_errno_t errno;
  int is_complete = 1;

  txn = RALEIGHFS_TRANSACTION(task->args[3].ptr);
  if (Z_UNLIKELY(task->state == OBJECT_SCHED_OPEN)) {
    object = raleighfs_obj_cache_get(fs, task->object.u64);
    if (raleighfs_object_is_open(fs, object)) {
      task->object.ptr = object;
      task->state = task->flags;
    }
  } else {
    object = RALEIGHFS_OBJECT(task->object.ptr);
  }

  /* let a commit finish before a transaction will begin */
  if (task->state != OBJECT_SCHED_COMMIT && object->pending_txn_id != 0) {
    Z_LOG_TRACE("Stuck with pending TXN-ID %u", object->pending_txn_id);
    /* TODO: Use a task sem? */
    z_global_add_pending_tasks(task);
    return;
  }

  operation_type = __sched_state_rwc_op[task->state];
  if (z_task_rwcsem_acquire(&(object->rwcsem), operation_type, task)) {
    return;
  }

  /* Execute the task */
  switch (task->state) {
    case OBJECT_SCHED_OPEN:
      task->object.ptr = object;
      errno = raleighfs_object_open(fs, object);
      if (errno) {
        raleighfs_object_rollback(fs, object);
      } else {
        task->state = task->flags;
        is_complete = 0;
      }
      break;
    case OBJECT_SCHED_READ:
      errno = __sched_task_read_func_exec(fs, txn, object, task);
      break;
    case OBJECT_SCHED_WRITE:
      errno = __sched_task_write_func_exec(fs, txn, object, task);
      if (errno) {
        if (txn != NULL) txn->state = RALEIGHFS_TXN_DONT_COMMIT;
        raleighfs_object_rollback(fs, object);
      } else {
        z_rwcsem_set_commit_flag(&(object->rwcsem.lock));
        task->state = OBJECT_SCHED_COMMIT;
        is_complete = 0;
      }
      break;
    case OBJECT_SCHED_COMMIT:
      errno = raleighfs_object_commit(fs, object);
      if (Z_UNLIKELY(errno)) {
        raleighfs_object_rollback(fs, object);
      }
      break;
  }

  z_task_rwcsem_release(&(object->rwcsem), operation_type, task, is_complete);

  if (is_complete) {
    __sched_task_notify_func_exec(fs, raleighfs_oid(object), errno, task);
    raleighfs_obj_cache_release(fs, object);
    raleighfs_transaction_release(fs, txn);
    z_task_free(task);
  }
}

/* ============================================================================
 *  PUBLIC Sched methods
 */
int raleighfs_exec_read (raleighfs_t *fs,
                         uint64_t txn_id, uint64_t oid,
                         raleighfs_read_func_t read_func,
                         raleighfs_notify_func_t notify_func,
                         void *req, void *resp,
                         void *udata, void *err_data)
{
  raleighfs_transaction_t *txn;
  raleighfs_errno_t errno;
  z_task_t *task;

  if ((errno = raleighfs_transaction_acquire(fs, txn_id, &txn))) {
    notify_func(fs, 0, errno, udata, err_data, req, resp);
    return(0);
  }

  task = z_task_alloc(__sched_object_task_exec);
  if (Z_MALLOC_IS_NULL(task)) {
    raleighfs_transaction_release(fs, txn);
    return(-1);
  }

  task->state = OBJECT_SCHED_OPEN;
  task->flags = OBJECT_SCHED_READ;
  task->context = fs;
  task->object.u64 = oid;
  task->request = req;
  task->response = resp;
  task->udata = udata;
  task->args[0].ptr = notify_func;
  task->args[1].ptr = err_data;
  task->args[2].ptr = read_func;
  task->args[3].ptr = txn;

  z_global_add_task(task);
  return(0);
}

int raleighfs_exec_write (raleighfs_t *fs,
                          uint64_t txn_id, uint64_t oid,
                          raleighfs_write_func_t write_func,
                          raleighfs_notify_func_t notify_func,
                          void *req, void *resp,
                          void *udata, void *err_data)
{
  raleighfs_transaction_t *txn;
  raleighfs_errno_t errno;
  z_task_t *task;

  if ((errno = raleighfs_transaction_acquire(fs, txn_id, &txn))) {
    notify_func(fs, 0, errno, udata, err_data, req, resp);
    return(0);
  }

  task = z_task_alloc(__sched_object_task_exec);
  if (Z_MALLOC_IS_NULL(task)) {
    raleighfs_transaction_release(fs, txn);
    return(-1);
  }

  task->state = OBJECT_SCHED_OPEN;
  task->flags = OBJECT_SCHED_WRITE;
  task->context = fs;
  task->object.u64 = oid;
  task->request = req;
  task->response = resp;
  task->udata = udata;
  task->args[0].ptr = notify_func;
  task->args[1].ptr = err_data;
  task->args[2].ptr = write_func;
  task->args[3].ptr = txn;

  z_global_add_task(task);
  return(0);
}
