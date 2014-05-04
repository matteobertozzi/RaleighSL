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

#include <zcl/task-rq.h>
#include <zcl/debug.h>

/* ============================================================================
 *  PRIVATE Group task-rq methods
 */
static void __task_rq_group_open (z_task_rq_t *self) {
  z_task_rq_group_t *group = z_container_of(self, z_task_rq_group_t, rq);
  z_vtask_queue_init(&(group->pending));
  z_vtask_queue_init(&(group->queue));
  group->running = 0;
  group->barrier = 0;
  group->waiting = 0;
}

static void __task_rq_group_close (z_task_rq_t *self) {
  z_task_rq_group_t *group = z_container_of(self, z_task_rq_group_t, rq);
  z_vtask_queue_cancel(&(group->pending));
  z_vtask_queue_cancel(&(group->queue));
}

static void __task_rq_group_add (z_task_rq_t *self, z_vtask_t *vtask) {
  z_task_rq_group_t *group = z_container_of(self, z_task_rq_group_t, rq);
  z_vtask_queue_push(&(group->queue), vtask);
}

static void __task_rq_group_readd (z_task_rq_t *self, z_vtask_t *vtask) {
  z_task_rq_group_t *group = z_container_of(self, z_task_rq_group_t, rq);
  z_vtask_queue_push(&(group->pending), vtask);
  group->has_barrier += vtask->flags.barrier;
}

static void __task_rq_group_remove (z_task_rq_t *self, z_vtask_t *vtask) {
  z_task_rq_group_t *group = z_container_of(self, z_task_rq_group_t, rq);
  if (group->pending.head != NULL)
    z_vtask_queue_remove(&(group->pending), vtask);
  if (group->queue.head != NULL)
    z_vtask_queue_remove(&(group->queue), vtask);
  group->has_barrier -= vtask->flags.barrier;
}

static z_vtask_t *__task_rq_group_fetch (z_task_rq_t *self) {
  z_task_rq_group_t *group = z_container_of(self, z_task_rq_group_t, rq);
  if (group->pending.head != NULL) {
    group->running += 1;
    return(z_vtask_queue_pop(&(group->pending)));
  }

  if (group->barrier || (group->queue.head->flags.barrier && group->running)) {
    Z_ASSERT(group->waiting == 0, "Already waiting");
    group->waiting = self->size;
    self->size = 0;
    return(NULL);
  }

  group->running += 1;
  group->barrier = group->queue.head->flags.barrier;
  group->has_barrier -= group->queue.head->flags.barrier;
  return(z_vtask_queue_pop(&(group->queue)));
}

static void __task_rq_group_fini (z_task_rq_t *self) {
  z_task_rq_group_t *group = z_container_of(self, z_task_rq_group_t, rq);
  if (--group->running == 0 && group->waiting) {
    self->size += group->waiting;
    group->waiting = 0;
  }
  group->barrier = 0;
}

const z_vtable_task_rq_t z_task_rq_group = {
  .fetch  = __task_rq_group_fetch,
  .add    = __task_rq_group_add,
  .readd  = __task_rq_group_readd,
  .remove = __task_rq_group_remove,
  .fini   = __task_rq_group_fini,
  .open   = __task_rq_group_open,
  .close  = __task_rq_group_close,
};

/* ============================================================================
 *  PUBLIC Task Group methods
 */
#include <zcl/semaphore.h>
#include <zcl/task.h>

struct task_latch {
  z_task_t vtask;
  z_latch_t latch;
};

static z_task_rstate_t __task_latch_func (z_task_t *vtask) {
  struct task_latch *task = z_container_of(vtask, struct task_latch, vtask);
  z_latch_release(&(task->latch));
  return(Z_TASK_COMPLETED);
}

void z_task_group_open (z_task_rq_group_t *self, uint8_t priority) {
  z_task_rq_open(&(self->rq), &z_task_rq_group, priority);
}

void z_task_group_close (z_task_rq_group_t *self) {
  z_task_rq_close(&(self->rq));
}

void z_task_group_add (z_task_rq_group_t *self, z_vtask_t *vtask) {
  z_task_rq_add(&(self->rq), vtask);
}

void z_task_group_add_barrier (z_task_rq_group_t *self, z_vtask_t *vtask) {
  vtask->flags.barrier = 1;
  z_task_rq_add(&(self->rq), vtask);
}

void z_task_group_wait (z_task_rq_group_t *self) {
  struct task_latch task;
  z_latch_open(&(task.latch));
  z_task_reset(&(task.vtask), __task_latch_func);
  z_task_group_add_barrier(self, &(task.vtask.vtask));
  z_latch_wait(&(task.latch), 1);
  z_latch_close(&(task.latch));
}
