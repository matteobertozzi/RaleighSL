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
#include <zcl/task.h>
#include <zcl/time.h>

/* ===========================================================================
 *  PRIVATE Task Tree
 */
static int __task_tree_key_compare (void *udata, const void *a, const void *b) {
  int64_t cmp = Z_TASK(a)->itime - Z_TASK(b)->itime;
  return((cmp < 0) ? -1 : (cmp) > 0 ? 1 : 0);
}

static void __task_tree_node_free (void *udata, void *object) {
  z_task_free(Z_TASK(object));
}

const z_tree_info_t __task_tree_info = {
  .plug        = &z_tree_red_black,
  .key_compare = __task_tree_key_compare,
  .data_free   = __task_tree_node_free,
  .user_data   = NULL,
};


/* ===========================================================================
 *  PUBLIC Task
 */
z_task_t *z_task_alloc (z_task_func_t func) {
  z_task_t *task;

  task = z_memory_struct_alloc(z_global_memory(), z_task_t);
  if (Z_MALLOC_IS_NULL(task))
    return(NULL);

  task->__node__.child[0] = Z_TREE_NODE(task);
  task->__node__.child[1] = Z_TREE_NODE(task);
  task->__node__.data = task;
  task->__node__.balance = 0;

  task->itime = z_time_millis();
  task->func = func;

  return(task);
}

void z_task_free (z_task_t *task) {
  z_memory_struct_free(z_global_memory(), z_task_t, task);
}

void z_task_reset_time(z_task_t *task) {
  task->itime = z_time_millis();
}

/* ===========================================================================
 *  PUBLIC Task queue
 */
#define __task_prev(x)     ((x)->__node__.child[0])
#define __task_next(x)     ((x)->__node__.child[1])

void z_task_queue_open (z_task_queue_t *self) {
  self->head = NULL;
}

void z_task_queue_close (z_task_queue_t *self) {
}

void z_task_queue_push (z_task_queue_t *self, z_task_t *task) {
  if (task == NULL)
    return;

  if (self->head == NULL) {
    self->head = task;
  } else {
    z_task_t *iprev = Z_TASK(__task_prev(self->head));
    z_task_t *inext = self->head;
    __task_next(task) = Z_TREE_NODE(inext);
    __task_prev(task) = Z_TREE_NODE(iprev);
    __task_prev(inext) = Z_TREE_NODE(task);
    __task_next(iprev) = Z_TREE_NODE(task);
  }
}

z_task_t *z_task_queue_pop (z_task_queue_t *self) {
  if (self->head != NULL) {
    z_task_t *task = self->head;
    z_task_t *iprev = Z_TASK(__task_prev(task));
    z_task_t *inext = Z_TASK(__task_next(task));

    self->head = (inext != task) ? inext : NULL;

    __task_prev(inext) = Z_TREE_NODE(iprev);
    __task_next(iprev) = Z_TREE_NODE(inext);

    __task_prev(task) = Z_TREE_NODE(task);
    __task_next(task) = Z_TREE_NODE(task);
    return(task);
  }
  return(NULL);
}

z_task_t *z_task_queue_drain (z_task_queue_t *self) {
  z_task_t *task = self->head;
  self->head = NULL;
  return(task);
}

/* ===========================================================================
 *  PUBLIC Task Tree
 */
void z_task_tree_open (z_task_tree_t *self) {
  self->root = NULL;
}

void z_task_tree_close (z_task_tree_t *self) {
}

void z_task_tree_push (z_task_tree_t *self, z_task_t *task) {
  if (task != NULL) {
    z_task_t *next;
    do {
      next = Z_TASK(__task_next(task));
      z_tree_node_attach(&__task_tree_info, &(self->root), Z_TREE_NODE(task));
      task = next;
    } while (task != next);
  }
}

z_task_t *z_task_tree_pop (z_task_tree_t *self) {
  if (self->root != NULL) {
    z_task_t *task = Z_TASK(z_tree_node_detach_min(&__task_tree_info, &(self->root)));
    task->__node__.child[0] = Z_TREE_NODE(task);
    task->__node__.child[1] = Z_TREE_NODE(task);
    return(task);
  }
  return(NULL);
}

/* ===========================================================================
 *  PUBLIC Task RWC-Semaphore
 */
static void __task_rwcsem_add (z_task_rwcsem_t *self,
                               z_rwcsem_op_t operation_type,
                               z_task_t *task)
{
  switch (operation_type) {
    case Z_RWCSEM_READ:
      z_task_queue_push(&(self->readq), task);
      break;
    case Z_RWCSEM_WRITE:
      z_task_queue_push(&(self->writeq), task);
      break;
    case Z_RWCSEM_COMMIT:
      z_task_queue_push(&(self->commitq), task);
      break;
    case Z_RWCSEM_LOCK:
      z_task_queue_push(&(self->lockq), task);
      break;
  }
}

static void __task_rwcsem_wake (z_task_rwcsem_t *self,
                                z_task_t *tasks[4],
                                uint32_t state)
{
  if (state & Z_RWCSEM_READABLE)   tasks[0] = z_task_queue_drain(&(self->readq));
  if (state & Z_RWCSEM_WRITABLE)   tasks[1] = z_task_queue_drain(&(self->writeq));
  if (state & Z_RWCSEM_COMMITABLE) tasks[2] = z_task_queue_drain(&(self->commitq));
  if (state & Z_RWCSEM_LOCKABLE)   tasks[3] = z_task_queue_drain(&(self->lockq));
}

int z_task_rwcsem_open (z_task_rwcsem_t *self) {
  z_rwcsem_init(&(self->lock));
  z_spin_alloc(&(self->wlock));
  z_task_queue_open(&(self->readq));
  z_task_queue_open(&(self->writeq));
  z_task_queue_open(&(self->commitq));
  z_task_queue_open(&(self->lockq));
  return(0);
}

void z_task_rwcsem_close (z_task_rwcsem_t *self) {
  z_task_queue_close(&(self->readq));
  z_task_queue_close(&(self->writeq));
  z_task_queue_close(&(self->commitq));
  z_task_queue_close(&(self->lockq));
  z_spin_free(&(self->wlock));
}

int z_task_rwcsem_acquire (z_task_rwcsem_t *self,
                           z_rwcsem_op_t operation_type,
                           z_task_t *task)
{
  if (!z_rwcsem_try_acquire(&(self->lock), operation_type)) {
    /* Add the task to the waiting queue */
    z_lock(&(self->wlock), z_spin, {
      __task_rwcsem_add(self, operation_type, task);
      /* get state and wake? */
    });
    return(1);
  }
  return(0);
}

void z_task_rwcsem_release (z_task_rwcsem_t *self,
                            z_rwcsem_op_t operation_type,
                            z_task_t *task,
                            int is_complete)
{
  z_task_t *wake[4] = {NULL, NULL, NULL, NULL}; /* {Read, Write, Commit, Lock} */
  uint32_t state;

  state = z_rwcsem_release(&(self->lock), operation_type);

  /* Wake up the waiting tasks */
  z_lock(&(self->wlock), z_spin, {
    /* self->state = state; */
    __task_rwcsem_wake(self, wake, state);
  });

  /* Add pending tasks to the dispatcher */
  if (is_complete) {
    z_global_add_pending_ntasks(4, wake[0], wake[1], wake[2], wake[3]);
  } else {
    z_global_add_pending_ntasks(5, task, wake[0], wake[1], wake[2], wake[3]);
  }
}
