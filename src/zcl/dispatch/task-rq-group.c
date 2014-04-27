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
}

static void __task_rq_group_close (z_task_rq_t *self) {
  z_task_rq_group_t *group = z_container_of(self, z_task_rq_group_t, rq);
  Z_LOG_WARN("TODO: Free pending tasks of RQ-Group %p", group);
}

static void __task_rq_group_add (z_task_rq_t *self, z_vtask_t *vtask) {
  z_task_rq_group_t *group = z_container_of(self, z_task_rq_group_t, rq);
  z_vtask_queue_push(&(group->queue), vtask);
}

static void __task_rq_group_readd (z_task_rq_t *self, z_vtask_t *vtask) {
  z_task_rq_group_t *group = z_container_of(self, z_task_rq_group_t, rq);
  z_vtask_queue_push(&(group->pending), vtask);
}

static z_vtask_t *__task_rq_group_fetch (z_task_rq_t *self) {
  z_task_rq_group_t *group = z_container_of(self, z_task_rq_group_t, rq);
  if (group->pending.head != NULL) {
    group->running += 1;
    return(z_vtask_queue_pop(&(group->pending)));
  }
  if (group->barrier || (group->queue.head->flags.barrier && group->running)) {
    /* TODO: This should be in wait */
    self->size += 1;
    return(NULL);
  }
  group->running += 1;
  group->barrier = group->queue.head->flags.barrier;
  return(z_vtask_queue_pop(&(group->queue)));
}

static void __task_rq_group_fini (z_task_rq_t *self) {
  z_task_rq_group_t *group = z_container_of(self, z_task_rq_group_t, rq);
  group->running -= 1;
  group->barrier = 0;
}

const z_vtable_task_rq_t z_task_rq_group = {
  .open   = __task_rq_group_open,
  .close  = __task_rq_group_close,
  .add    = __task_rq_group_add,
  .readd  = __task_rq_group_readd,
  .fetch  = __task_rq_group_fetch,
  .fini   = __task_rq_group_fini,
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

static void __task_latch_func (z_task_t *vtask) {
  struct task_latch *task = z_container_of(vtask, struct task_latch, vtask);
  z_latch_release(&(task->latch));
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
  z_latch_timed_wait(&(task.latch), 1, 2);
  z_latch_wait(&(task.latch), 1);
  z_latch_close(&(task.latch));
}
