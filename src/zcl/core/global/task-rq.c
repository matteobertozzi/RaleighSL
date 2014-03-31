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

#include <zcl/task.h>

/* ============================================================================
 *  PRIVATE FIFO task-rq methods
 */
static void __task_rq_fifo_open (z_task_rq_t *self) {
  z_task_rq_fifo_t *fifo = z_container_of(self, z_task_rq_fifo_t, rq);
  z_task_tree_open(&(fifo->pending));
  z_task_queue_open(&(fifo->queue));
  fifo->seqid = 0;
}

static void __task_rq_fifo_close (z_task_rq_t *self) {
  z_task_rq_fifo_t *fifo = z_container_of(self, z_task_rq_fifo_t, rq);
  z_task_tree_close(&(fifo->pending));
  z_task_queue_close(&(fifo->queue));
}

static void __task_rq_fifo_add (z_task_rq_t *self, z_task_t *task) {
  z_task_rq_fifo_t *fifo = z_container_of(self, z_task_rq_fifo_t, rq);
  if (z_task_seqid(task) == 0) {
    z_task_set_seqid(task, ++(fifo->seqid));
    z_task_queue_push(&(fifo->queue), task);
  } else {
    z_task_tree_push(&(fifo->pending), task);
  }
  self->size += 1;
}

static z_task_t *__task_rq_fifo_fetch (z_task_rq_t *self) {
  z_task_rq_fifo_t *fifo = z_container_of(self, z_task_rq_fifo_t, rq);
  z_task_t *task;
  self->size -= 1;
  if ((task = z_task_tree_pop(&(fifo->pending))) != NULL)
    return(task);
  return(z_task_queue_pop(&(fifo->queue)));
}

const z_vtable_task_rq_t z_task_rq_fifo = {
  .open   = __task_rq_fifo_open,
  .close  = __task_rq_fifo_close,
  .add    = __task_rq_fifo_add,
  .fetch  = __task_rq_fifo_fetch,
};

/* ============================================================================
 *  PRIVATE fair task-rq methods
 */
static void __task_rq_fair_open (z_task_rq_t *self) {
  z_task_rq_fair_t *fair = z_container_of(self, z_task_rq_fair_t, rq);
  fair->queue = NULL;
  fair->seqid = 0;
}

static void __task_rq_fair_close (z_task_rq_t *self) {
}

static void __task_rq_fair_add (z_task_rq_t *self, z_task_t *task) {
  z_task_rq_fair_t *fair = z_container_of(self, z_task_rq_fair_t, rq);
  if (z_task_seqid(task) == 0) {
    z_task_set_seqid(task, ++(fair->seqid));
  }
  z_tree_node_attach(&z_task_vtime_tree_info, &(fair->queue), &(task->sys_node), NULL);
  self->size += 1;
}

static z_task_t *__task_rq_fair_fetch (z_task_rq_t *self) {
  z_task_rq_fair_t *fair = z_container_of(self, z_task_rq_fair_t, rq);
  z_tree_node_t *node;
  z_task_t *task;

  self->size -= 1;
  node = z_tree_node_detach_min(&z_task_vtime_tree_info, &(fair->queue));
  task = z_container_of(node, z_task_t, sys_node);
  z_task_set_vtime(task, 1 + z_task_vtime(task));
  return(task);
}

const z_vtable_task_rq_t z_task_rq_fair = {
  .open   = __task_rq_fair_open,
  .close  = __task_rq_fair_close,
  .add    = __task_rq_fair_add,
  .fetch  = __task_rq_fair_fetch,
};

/* ============================================================================
 *  PUBLIC task-rq methods
 */
void z_task_rq_open (z_task_rq_t *self,
                     const z_vtable_task_rq_t *vtable,
                     uint8_t priority)
{
  z_dlink_init(&(self->link));
  self->prio = priority;
  self->size = 0;
  self->vtable = vtable;
  vtable->open(self);
}
