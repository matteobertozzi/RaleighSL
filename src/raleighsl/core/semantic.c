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

#include <raleighsl/semantic.h>
#include <raleighsl/exec.h>

#include <zcl/global.h>
#include <zcl/debug.h>

#include "private.h"

/* ============================================================================
 *  PRIVATE Semantic methods
 */
int raleighsl_semantic_alloc (raleighsl_t *fs) {
  raleighsl_semantic_t *semantic = &(fs->semantic);
  semantic->next_oid = 1;
  z_task_rwcsem_open(&(semantic->rwcsem));
  return(0);
}

void raleighsl_semantic_free (raleighsl_t *fs) {
  raleighsl_semantic_t *semantic = &(fs->semantic);
  z_task_rwcsem_close(&(semantic->rwcsem));
}

/* ============================================================================
 *  PUBLIC Semantic methods
 */
raleighsl_errno_t raleighsl_semantic_create (raleighsl_t *fs,
                                             const raleighsl_object_plug_t *plug,
                                             const z_bytes_ref_t *name,
                                             uint64_t *oid)
{
  raleighsl_errno_t errno;

  /* Verify that the object is not alrady present */
  switch ((errno = __semantic_call_required(fs, lookup, name, oid))) {
    case RALEIGHSL_ERRNO_OBJECT_NOT_FOUND:
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
  if ((errno = raleighsl_object_create(fs, plug, *oid))) {
    return(errno);
  }

  /* __observer_notify_create(fs, name); */
  return(RALEIGHSL_ERRNO_NONE);
}

raleighsl_errno_t raleighsl_semantic_open (raleighsl_t *fs,
                                           const z_bytes_ref_t *name,
                                           uint64_t *oid)
{
  raleighsl_object_t *object;
  raleighsl_errno_t errno;

  /* Semantic open, lookup metadata */
  if ((errno = __semantic_call_required(fs, lookup, name, oid))) {
    return(errno);
  }

  if ((object = raleighsl_obj_cache_get(fs, *oid)) == NULL) {
    return(RALEIGHSL_ERRNO_NO_MEMORY);
  }

  if ((errno = raleighsl_object_open(fs, object))) {
    raleighsl_obj_cache_release(fs, object);
    return(errno);
  }

  raleighsl_obj_cache_release(fs, object);

  /* __observer_notify_open(fs, name); */
  return(RALEIGHSL_ERRNO_NONE);
}

raleighsl_errno_t raleighsl_semantic_unlink (raleighsl_t *fs,
                                             const z_bytes_ref_t *name)
{
  raleighsl_object_t *object;
  raleighsl_errno_t errno;
  uint64_t oid;

  if ((errno = __semantic_call_required(fs, unlink, name, &oid))) {
    return(errno);
  }

  if ((object = raleighsl_obj_cache_get(fs, oid)) == NULL) {
    return(RALEIGHSL_ERRNO_NO_MEMORY);
  }

  if ((errno = __object_call_unrequired(fs, object, unlink))) {
    raleighsl_obj_cache_release(fs, object);
    return(errno);
  }

  raleighsl_obj_cache_release(fs, object);

  /* __observer_notify_unlink(fs, object->name); */
  return(RALEIGHSL_ERRNO_NONE);
}

raleighsl_errno_t raleighsl_semantic_rename (raleighsl_t *fs,
                                             const z_bytes_ref_t *old_name,
                                             const z_bytes_ref_t *new_name)
{
  raleighsl_errno_t errno;
  uint64_t oid;

  /* Verify that the 'new_name' is not alrady present */
  switch ((errno = __semantic_call_required(fs, lookup, new_name, &oid))) {
    case RALEIGHSL_ERRNO_OBJECT_NOT_FOUND:
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
  return(RALEIGHSL_ERRNO_NONE);
}

/* ============================================================================
 *  RaleighSL Semantic Scheduler
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
  ((raleighsl_create_func_t)((task)->args[2].ptr))                      \
    (fs, plug, (task)->udata)

#define __sched_task_modify_func_exec(fs, task)                         \
  ((raleighsl_modify_func_t)((task)->args[2].ptr))                      \
    (fs, (task)->udata)

#define __sched_task_lookup_func_exec(fs, task)                         \
  ((raleighsl_lookup_func_t)((task)->args[2].ptr))                      \
    (fs, (task)->udata)

#define __sched_task_notify_func_exec(fs, oid, errno, task)             \
  ((raleighsl_notify_func_t)((task)->args[0].ptr))                      \
    (fs, oid, errno, (task)->udata, ((task)->args[1].ptr))

static void __sched_semantic_task_exec (z_task_t *task) {
  raleighsl_t *fs = RALEIGHSL(task->context);
  raleighsl_errno_t errno;
  z_rwcsem_op_t op_type;
  int keep_running = 0;
  int is_complete = 1;

  op_type = __sched_state_rwc_op[task->state];
  if (z_task_rwcsem_acquire(&(fs->semantic.rwcsem), op_type, task))
    return;

  do {
    keep_running = 0;
    is_complete = 1;

    op_type = __sched_state_rwc_op[task->state];
    switch (task->state) {
      case SEMANTIC_SCHED_CREATE:
        errno = __sched_task_create_func_exec(fs, RALEIGHSL_OBJECT_PLUG(task->args[3].ptr), task);
        if (errno) {
          raleighsl_semantic_rollback(fs);
        } else {
          z_rwcsem_set_commit_flag(&(fs->semantic.rwcsem.lock));
          is_complete = 0;
          task->state = SEMANTIC_SCHED_COMMIT;
          keep_running = z_rwcsem_try_switch(&(fs->semantic.rwcsem.lock), op_type, __sched_state_rwc_op[task->state]);
        }
        break;
      case SEMANTIC_SCHED_LOOKUP:
        errno = __sched_task_lookup_func_exec(fs, task);
        break;
      case SEMANTIC_SCHED_UNLINK:
      case SEMANTIC_SCHED_RENAME:
        errno = __sched_task_modify_func_exec(fs, task);
        if (errno) {
          raleighsl_semantic_rollback(fs);
        } else {
          z_rwcsem_set_commit_flag(&(fs->semantic.rwcsem.lock));
          is_complete = 0;
          task->state = SEMANTIC_SCHED_COMMIT;
          keep_running = z_rwcsem_try_switch(&(fs->semantic.rwcsem.lock), op_type, __sched_state_rwc_op[task->state]);
        }
        break;
      case SEMANTIC_SCHED_COMMIT:
        errno = raleighsl_semantic_commit(fs);
        if (Z_UNLIKELY(errno)) {
          raleighsl_semantic_rollback(fs);
        }
        break;
    }
  } while (keep_running);
  z_task_rwcsem_release(&(fs->semantic.rwcsem), op_type, task, is_complete);

  if (is_complete) {
    __sched_task_notify_func_exec(fs, 0, errno, task);
    z_task_free(task);
  }
}

int raleighsl_exec_create (raleighsl_t *fs,
                           const raleighsl_object_plug_t *plug,
                           raleighsl_create_func_t create_func,
                           raleighsl_notify_func_t notify_func,
                           void *udata, void *err_data)
{
  z_task_t *task;

  task = z_task_alloc(__sched_semantic_task_exec);
  if (Z_MALLOC_IS_NULL(task))
    return(1);

  task->state = SEMANTIC_SCHED_CREATE;
  task->flags = SEMANTIC_SCHED_CREATE;
  task->context = fs;
  task->udata = udata;
  task->args[0].ptr = notify_func;
  task->args[1].ptr = err_data;
  task->args[2].ptr = create_func;
  task->args[3].ptr = (void *)plug;

  z_global_add_task(task);
  return(0);
}

static int raleighsl_exec_modify (raleighsl_t *fs,
                                  enum semantic_sched_state state,
                                  raleighsl_modify_func_t modify_func,
                                  raleighsl_notify_func_t notify_func,
                                  void *udata, void *err_data)
{
  z_task_t *task;

  task = z_task_alloc(__sched_semantic_task_exec);
  if (Z_MALLOC_IS_NULL(task))
    return(1);

  task->state = state;
  task->flags = state;
  task->context = fs;
  task->udata = udata;
  task->args[0].ptr = notify_func;
  task->args[1].ptr = err_data;
  task->args[2].ptr = modify_func;

  z_global_add_task(task);
  return(0);
}

int raleighsl_exec_rename (raleighsl_t *fs,
                           raleighsl_modify_func_t modify_func,
                           raleighsl_notify_func_t notify_func,
                           void *udata, void *err_data)
{
  return(raleighsl_exec_modify(fs, SEMANTIC_SCHED_RENAME,
                               modify_func, notify_func,
                               udata, err_data));
}

int raleighsl_exec_unlink (raleighsl_t *fs,
                           raleighsl_modify_func_t modify_func,
                           raleighsl_notify_func_t notify_func,
                           void *udata, void *err_data)
{
  return(raleighsl_exec_modify(fs, SEMANTIC_SCHED_UNLINK,
                               modify_func, notify_func,
                               udata, err_data));
}

int raleighsl_exec_lookup (raleighsl_t *fs,
                           raleighsl_lookup_func_t lookup_func,
                           raleighsl_notify_func_t notify_func,
                           void *udata, void *err_data)
{
  z_task_t *task;

  task = z_task_alloc(__sched_semantic_task_exec);
  if (Z_MALLOC_IS_NULL(task))
    return(1);

  task->state = SEMANTIC_SCHED_LOOKUP;
  task->flags = SEMANTIC_SCHED_LOOKUP;
  task->context = fs;
  task->udata = udata;
  task->args[0].ptr = notify_func;
  task->args[1].ptr = err_data;
  task->args[2].ptr = lookup_func;

  z_global_add_task(task);
  return(0);
}
