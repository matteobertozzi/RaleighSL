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
 *  PRIVATE FIFO task-rq methods
 */
static void __task_rq_fifo_open (z_task_rq_t *self) {
  z_task_rq_fifo_t *fifo = z_container_of(self, z_task_rq_fifo_t, rq);
  z_vtask_tree_init(&(fifo->pending));
  z_vtask_queue_init(&(fifo->queue));
}

static void __task_rq_fifo_close (z_task_rq_t *self) {
  z_task_rq_fifo_t *fifo = z_container_of(self, z_task_rq_fifo_t, rq);
  z_vtask_tree_cancel(&(fifo->pending));
  z_vtask_queue_cancel(&(fifo->queue));
}

static void __task_rq_fifo_add (z_task_rq_t *self, z_vtask_t *vtask) {
  z_task_rq_fifo_t *fifo = z_container_of(self, z_task_rq_fifo_t, rq);
  z_vtask_queue_push(&(fifo->queue), vtask);
}

static void __task_rq_fifo_readd (z_task_rq_t *self, z_vtask_t *vtask) {
  z_task_rq_fifo_t *fifo = z_container_of(self, z_task_rq_fifo_t, rq);
  z_vtask_tree_push(&(fifo->pending), vtask);
}

static void __task_rq_fifo_remove (z_task_rq_t *self, z_vtask_t *vtask) {
  z_task_rq_fifo_t *fifo = z_container_of(self, z_task_rq_fifo_t, rq);
  if (fifo->pending.root != NULL)
    z_vtask_tree_remove(&(fifo->pending), vtask);
  if (fifo->queue.head != NULL)
    z_vtask_queue_remove(&(fifo->queue), vtask);
}

static z_vtask_t *__task_rq_fifo_fetch (z_task_rq_t *self) {
  z_task_rq_fifo_t *fifo = z_container_of(self, z_task_rq_fifo_t, rq);
  if (fifo->pending.root != NULL) {
    return(z_vtask_tree_pop(&(fifo->pending)));
  } else {
    z_vtask_t *vtask = fifo->queue.head;
    if (vtask->flags.type == Z_VTASK_TYPE_RQ) {
      vtask->vtime += vtask->priority;
      self->size += 1;
      return(vtask);
    }
  }
  return(z_vtask_queue_pop(&(fifo->queue)));
}

const z_vtable_task_rq_t z_task_rq_fifo = {
  .open   = __task_rq_fifo_open,
  .close  = __task_rq_fifo_close,
  .add    = __task_rq_fifo_add,
  .readd  = __task_rq_fifo_readd,
  .remove = __task_rq_fifo_remove,
  .fetch  = __task_rq_fifo_fetch,
  .fini   = NULL,
};
