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
 *  PRIVATE Task RWC-Semaphore
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

/* ===========================================================================
 *  PUBLIC Task RWC-Semaphore
 */
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
    z_global_add_ntasks(4, wake[0], wake[1], wake[2], wake[3]);
  } else {
    z_global_add_ntasks(5, task, wake[0], wake[1], wake[2], wake[3]);
  }
}
