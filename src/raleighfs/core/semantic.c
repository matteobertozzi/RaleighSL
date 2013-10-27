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

#include <raleighfs/semantic.h>
#include <raleighfs/exec.h>

#include <zcl/global.h>
#include <zcl/debug.h>

#include "private.h"

/* ============================================================================
 *  PRIVATE Semantic methods
 */
int raleighfs_semantic_alloc (raleighfs_t *fs) {
  raleighfs_semantic_t *semantic = &(fs->semantic);
  semantic->next_oid = 1;
  z_task_rwcsem_open(&(semantic->rwcsem));
  return(0);
}

void raleighfs_semantic_free (raleighfs_t *fs) {
  raleighfs_semantic_t *semantic = &(fs->semantic);
  z_task_rwcsem_close(&(semantic->rwcsem));
}

/* ============================================================================
 *  PUBLIC Semantic methods
 */
raleighfs_errno_t raleighfs_semantic_create (raleighfs_t *fs,
                                             const raleighfs_object_plug_t *plug,
                                             z_bytes_t *name,
                                             uint64_t *oid)
{
  raleighfs_errno_t errno;

  /* Verify that the object is not alrady present */
  switch ((errno = __semantic_call_required(fs, lookup, name, oid))) {
    case RALEIGHFS_ERRNO_OBJECT_NOT_FOUND:
      /* Go ahead with create */
      break;
    default:
      return(errno);
  }

  /* Semantic create metadata */
  *oid = fs->semantic.next_oid++;
  if ((errno = __semantic_call_required(fs, create, name, *oid))) {
    return(errno);
  }

  /* Object create */
  if ((errno = raleighfs_object_create(fs, plug, *oid))) {
    return(errno);
  }

  /* __observer_notify_create(fs, name); */
  return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_errno_t raleighfs_semantic_open (raleighfs_t *fs,
                                           const z_bytes_t *name,
                                           uint64_t *oid)
{
  raleighfs_object_t *object;
  raleighfs_errno_t errno;

  /* Semantic open, lookup metadata */
  if ((errno = __semantic_call_required(fs, lookup, name, oid))) {
    return(errno);
  }

  if ((object = raleighfs_obj_cache_get(fs, *oid)) == NULL) {
    return(RALEIGHFS_ERRNO_NO_MEMORY);
  }

  if ((errno = raleighfs_object_open(fs, object))) {
    raleighfs_obj_cache_release(fs, object);
    return(errno);
  }

  raleighfs_obj_cache_release(fs, object);

  /* __observer_notify_open(fs, name); */
  return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_errno_t raleighfs_semantic_unlink (raleighfs_t *fs,
                                             const z_bytes_t *name)
{
  raleighfs_object_t *object;
  raleighfs_errno_t errno;
  uint64_t oid;

  if ((errno = __semantic_call_required(fs, unlink, name, &oid))) {
    return(errno);
  }

  if ((object = raleighfs_obj_cache_get(fs, oid)) == NULL) {
    return(RALEIGHFS_ERRNO_NO_MEMORY);
  }

  if ((errno = __object_call_unrequired(fs, object, unlink))) {
    raleighfs_obj_cache_release(fs, object);
    return(errno);
  }

  raleighfs_obj_cache_release(fs, object);

  /* __observer_notify_unlink(fs, object->name); */
  return(RALEIGHFS_ERRNO_NONE);
}

raleighfs_errno_t raleighfs_semantic_rename (raleighfs_t *fs,
                                             const z_bytes_t *old_name,
                                             z_bytes_t *new_name)
{
  raleighfs_errno_t errno;
  uint64_t oid;

  /* Verify that the 'new_name' is not alrady present */
  switch ((errno = __semantic_call_required(fs, lookup, new_name, &oid))) {
    case RALEIGHFS_ERRNO_OBJECT_NOT_FOUND:
      /* Go ahead with create */
      break;
    default:
      return(errno);
  }

  /* Remove the 'old_name' from the semantic layer */
  if ((errno = __semantic_call_required(fs, unlink, old_name, &oid))) {
    Z_LOG_TRACE("Unable to unlink old");
    return(errno);
  }

  /* Create the 'new_name' in the semantic layer */
  if ((errno = __semantic_call_required(fs, create, new_name, oid))) {
    Z_LOG_TRACE("Unable to create new");
    return(errno);
  }

  /* __observer_notify_rename(fs, old_name, new_name); */
  return(RALEIGHFS_ERRNO_NONE);
}

/* ============================================================================
 *  RaleighFS Semantic Scheduler
 */
enum semantic_sched_state {
  SEMANTIC_SCHED_CREATE = 0,
  SEMANTIC_SCHED_LOOKUP = 1,
  SEMANTIC_SCHED_UNLINK = 2,
  SEMANTIC_SCHED_RENAME = 3,
  SEMANTIC_SCHED_COMMIT = 4,
};

static z_rwcsem_op_t __sched_state_rwc_op[] = {
  [SEMANTIC_SCHED_CREATE] = Z_RWCSEM_WRITE,
  [SEMANTIC_SCHED_LOOKUP] = Z_RWCSEM_READ,
  [SEMANTIC_SCHED_UNLINK] = Z_RWCSEM_WRITE,
  [SEMANTIC_SCHED_RENAME] = Z_RWCSEM_WRITE,
  [SEMANTIC_SCHED_COMMIT] = Z_RWCSEM_COMMIT,
};

#define __sched_task_create_func_exec(fs, plug, task)                   \
  ((raleighfs_create_func_t)((task)->args[2].ptr))                      \
    (fs, plug, (task)->udata, (task)->request, (task)->response)

#define __sched_task_modify_func_exec(fs, task)                         \
  ((raleighfs_modify_func_t)((task)->args[2].ptr))                      \
    (fs, (task)->udata, (task)->request, (task)->response)

#define __sched_task_lookup_func_exec(fs, task)                         \
  ((raleighfs_lookup_func_t)((task)->args[2].ptr))                      \
    (fs, (task)->udata, (task)->request, (task)->response)

#define __sched_task_notify_func_exec(fs, oid, errno, task)             \
  ((raleighfs_notify_func_t)((task)->args[0].ptr))                      \
    (fs, oid, errno, (task)->udata, ((task)->args[1].ptr),              \
      (task)->request, (task)->response)

static void __sched_semantic_task_exec (z_task_t *task) {
  raleighfs_t *fs = RALEIGHFS(task->context);
  z_rwcsem_op_t operation_type;
  raleighfs_errno_t errno;
  int is_complete = 1;

  operation_type = __sched_state_rwc_op[task->state];
  if (z_task_rwcsem_acquire(&(fs->semantic.rwcsem), operation_type, task))
    return;

  switch (task->state) {
    case SEMANTIC_SCHED_CREATE:
      errno = __sched_task_create_func_exec(fs, RALEIGHFS_OBJECT_PLUG(task->args[3].ptr), task);
      if (errno) {
        raleighfs_semantic_rollback(fs);
      } else {
        z_rwcsem_set_commit_flag(&(fs->semantic.rwcsem.lock));
        task->state = SEMANTIC_SCHED_COMMIT;
        is_complete = 0;
      }
      break;
    case SEMANTIC_SCHED_LOOKUP:
      errno = __sched_task_lookup_func_exec(fs, task);
      break;
    case SEMANTIC_SCHED_UNLINK:
    case SEMANTIC_SCHED_RENAME:
      errno = __sched_task_modify_func_exec(fs, task);
      if (errno) {
        raleighfs_semantic_rollback(fs);
      } else {
        z_rwcsem_set_commit_flag(&(fs->semantic.rwcsem.lock));
        task->state = SEMANTIC_SCHED_COMMIT;
        is_complete = 0;
      }
      break;
    case SEMANTIC_SCHED_COMMIT:
      errno = raleighfs_semantic_commit(fs);
      if (Z_UNLIKELY(errno)) {
        raleighfs_semantic_rollback(fs);
      }
      break;
  }

  z_task_rwcsem_release(&(fs->semantic.rwcsem), operation_type, task, is_complete);

  if (is_complete) {
    __sched_task_notify_func_exec(fs, 0, errno, task);
    z_task_free(task);
  }
}

int raleighfs_exec_create (raleighfs_t *fs,
                           const raleighfs_object_plug_t *plug,
                           raleighfs_create_func_t create_func,
                           raleighfs_notify_func_t notify_func,
                           void *req, void *resp,
                           void *udata, void *err_data)
{
  z_task_t *task;

  task = z_task_alloc(__sched_semantic_task_exec);
  if (Z_MALLOC_IS_NULL(task))
    return(1);

  task->state = SEMANTIC_SCHED_CREATE;
  task->flags = SEMANTIC_SCHED_CREATE;
  task->context = fs;
  task->request = req;
  task->response = resp;
  task->udata = udata;
  task->args[0].ptr = notify_func;
  task->args[1].ptr = err_data;
  task->args[2].ptr = create_func;
  task->args[3].ptr = (void *)plug;

  z_global_add_task(task);
  return(0);
}

static int raleighfs_exec_modify (raleighfs_t *fs,
                                  enum semantic_sched_state state,
                                  raleighfs_modify_func_t modify_func,
                                  raleighfs_notify_func_t notify_func,
                                  void *req, void *resp,
                                  void *udata, void *err_data)
{
  z_task_t *task;

  task = z_task_alloc(__sched_semantic_task_exec);
  if (Z_MALLOC_IS_NULL(task))
    return(1);

  task->state = state;
  task->flags = state;
  task->context = fs;
  task->request = req;
  task->response = resp;
  task->udata = udata;
  task->args[0].ptr = notify_func;
  task->args[1].ptr = err_data;
  task->args[2].ptr = modify_func;

  z_global_add_task(task);
  return(0);
}

int raleighfs_exec_rename (raleighfs_t *fs,
                           raleighfs_modify_func_t modify_func,
                           raleighfs_notify_func_t notify_func,
                           void *req, void *resp,
                           void *udata, void *err_data)
{
  return(raleighfs_exec_modify(fs, SEMANTIC_SCHED_RENAME,
                               modify_func, notify_func,
                               req, resp, udata, err_data));
}

int raleighfs_exec_unlink (raleighfs_t *fs,
                           raleighfs_modify_func_t modify_func,
                           raleighfs_notify_func_t notify_func,
                           void *req, void *resp,
                           void *udata, void *err_data)
{
  return(raleighfs_exec_modify(fs, SEMANTIC_SCHED_UNLINK,
                               modify_func, notify_func,
                               req, resp, udata, err_data));
}

int raleighfs_exec_lookup (raleighfs_t *fs,
                           raleighfs_lookup_func_t lookup_func,
                           raleighfs_notify_func_t notify_func,
                           void *req, void *resp,
                           void *udata, void *err_data)
{
  z_task_t *task;

  task = z_task_alloc(__sched_semantic_task_exec);
  if (Z_MALLOC_IS_NULL(task))
    return(1);

  task->state = SEMANTIC_SCHED_LOOKUP;
  task->flags = SEMANTIC_SCHED_LOOKUP;
  task->context = fs;
  task->request = req;
  task->response = resp;
  task->udata = udata;
  task->args[0].ptr = notify_func;
  task->args[1].ptr = err_data;
  task->args[2].ptr = lookup_func;

  z_global_add_task(task);
  return(0);
}
