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
static int __task_tree_compare (void *udata, const void *a, const void *b) {
  const z_task_t *ea = z_container_of(a, const z_task_t, __node__);
  const z_task_t *eb = z_container_of(b, const z_task_t, __node__);
  return(z_cmp(ea->itime, eb->itime));
}

static void __task_tree_node_free (void *udata, void *obj) {
  z_task_t *task = z_container_of(obj, z_task_t, __node__);
  z_task_free(task);
}

static const z_tree_info_t __task_tree_info = {
  .plug         = &z_tree_avl,
  .node_compare = __task_tree_compare,
  .key_compare  = __task_tree_compare,
  .node_free    = __task_tree_node_free,
};

/* ===========================================================================
 *  PUBLIC Task
 */
z_task_t *z_task_alloc (z_task_func_t func) {
  z_task_t *task;

  task = z_memory_struct_alloc(z_global_memory(), z_task_t);
  if (Z_MALLOC_IS_NULL(task))
    return(NULL);

  task->itime = 0;
  task->func = func;
  return(task);
}

void z_task_free (z_task_t *task) {
  z_memory_struct_free(z_global_memory(), z_task_t, task);
}

/* ===========================================================================
 *  PUBLIC Task queue
 */
void z_task_queue_open (z_task_queue_t *self) {
  self->head = NULL;
}

void z_task_queue_close (z_task_queue_t *self) {
}

void z_task_queue_push (z_task_queue_t *self, z_task_t *task) {
  if (task != NULL) {
    z_tree_node_t *node = &(task->__node__);
    if (self->head != NULL) {
      z_tree_node_t *head = &(self->head->__node__);
      z_tree_node_t *tail = head->child[1];

      tail->child[0] = node;
      head->child[1] = node;
      node->child[0] = NULL;
      node->child[1] = NULL;
    } else {
      node->child[0] = NULL;
      node->child[1] = node;
      self->head = task;
    }
  }
}

z_task_t *z_task_queue_pop (z_task_queue_t *self) {
  if (self->head != NULL) {
    z_task_t *task = self->head;
    z_tree_node_t *head = &(self->head->__node__);
    z_tree_node_t *next = head->child[0];
    z_tree_node_t *tail = head->child[1];

    if ((self->head = z_container_of(next, z_task_t, __node__)) != NULL)
      next->child[1] = tail;

    task->__node__.child[0] = NULL;
    task->__node__.child[1] = NULL;
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
  while (task != NULL) {
    z_task_t *next = Z_TASK(task->__node__.child[0]);
    z_tree_node_attach(&__task_tree_info, &(self->root), &(task->__node__), NULL);
    task = next;
  }
}

z_task_t *z_task_tree_pop (z_task_tree_t *self) {
  if (self->root != NULL) {
    return(Z_TASK(z_tree_node_detach_min(&__task_tree_info, &(self->root))));
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

static void __task_rwcsem_runnables (z_task_rwcsem_t *self,
                                     z_task_t *tasks[4],
                                     uint32_t state)
{
  if (self->readq.head != NULL && z_rwcsem_is_readable(state))
    tasks[0] = z_task_queue_drain(&(self->readq));
  if (self->writeq.head != NULL && z_rwcsem_is_writable(state))
    tasks[1] = z_task_queue_drain(&(self->writeq));
  if (self->commitq.head != NULL && z_rwcsem_is_committable(state))
    tasks[2] = z_task_queue_drain(&(self->commitq));
  if (self->lockq.head != NULL && z_rwcsem_is_lockable(state))
    tasks[3] = z_task_queue_drain(&(self->lockq));
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
    __task_rwcsem_runnables(self, wake, state);
  });

  /* Add pending tasks to the dispatcher */
  if (is_complete) {
    z_global_add_pending_ntasks(4, wake[0], wake[1], wake[2], wake[3]);
  } else {
    z_global_add_pending_ntasks(5, task, wake[0], wake[1], wake[2], wake[3]);
  }
}
