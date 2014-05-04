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
 *  PRIVATE Round-Robin task-rq methods
 */
static void __task_rq_rr_open (z_task_rq_t *self) {
  z_task_rq_rr_t *rr = z_container_of(self, z_task_rq_rr_t, rq);
  z_vtask_queue_init(&(rr->queue));
  rr->prio_sum = 1;
  rr->quantum = 0;
}

static void __task_rq_rr_close (z_task_rq_t *self) {
  z_task_rq_rr_t *rr = z_container_of(self, z_task_rq_rr_t, rq);
  z_vtask_queue_cancel(&(rr->queue));
}

static void __task_rq_rr_add (z_task_rq_t *self, z_vtask_t *vtask) {
  z_task_rq_rr_t *rr = z_container_of(self, z_task_rq_rr_t, rq);
  if (vtask->flags.type == Z_VTASK_TYPE_RQ) {
    rr->prio_sum += vtask->priority;
    vtask->flags.quantum = z_task_rq_quantum(rr, vtask->priority);
  }
  z_vtask_queue_push(&(rr->queue), vtask);
}

static void __task_rq_rr_remove (z_task_rq_t *self, z_vtask_t *vtask) {
  z_task_rq_rr_t *rr = z_container_of(self, z_task_rq_rr_t, rq);
  if (vtask->flags.type == Z_VTASK_TYPE_RQ) {
    rr->prio_sum -= vtask->priority;
  }
  z_vtask_queue_remove(&(rr->queue), vtask);
}

static z_vtask_t *__task_rq_rr_fetch (z_task_rq_t *self) {
  z_task_rq_rr_t *rr = z_container_of(self, z_task_rq_rr_t, rq);
  z_vtask_t *vtask = rr->queue.head;
  Z_ASSERT(vtask != NULL, "Null Task on %p size=%u", self, self->size);
  if (vtask->flags.type == Z_VTASK_TYPE_RQ) {
    if (--vtask->flags.quantum == 0) {
      vtask->flags.quantum = z_task_rq_quantum(rr, vtask->priority);
      z_vtask_queue_move_to_tail(&(rr->queue));
    }
    self->size += 1;
    return(vtask);
  }
  return(z_vtask_queue_pop(&(rr->queue)));
}

const z_vtable_task_rq_t z_task_rq_rr = {
  .fetch  = __task_rq_rr_fetch,
  .add    = __task_rq_rr_add,
  .readd  = __task_rq_rr_add,
  .remove = __task_rq_rr_remove,
  .fini   = NULL,

  .open   = __task_rq_rr_open,
  .close  = __task_rq_rr_close,
};
