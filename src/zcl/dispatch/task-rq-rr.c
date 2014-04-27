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
 *  PRIVATE task-sched methods
 */
#define __RQ_PRIO_SH        8

#define __rq_fairness(self, rq_prio)                                        \
  (((rq_prio) << __RQ_PRIO_SH) / (self)->prio_sum)

#define __rq_quantum(self, rq_prio)                                         \
  (1 + ((__rq_fairness(self, rq_prio) << (self)->quantum) >> __RQ_PRIO_SH))

/* ============================================================================
 *  PRIVATE Round-Robin task-rq methods
 */
static void __task_rq_rr_open (z_task_rq_t *self) {
  z_task_rq_rr_t *rr = z_container_of(self, z_task_rq_rr_t, rq);
  rr->prio_sum = 1;
  rr->quantum = 0;
}

static void __task_rq_rr_close (z_task_rq_t *self) {
  z_task_rq_rr_t *rr = z_container_of(self, z_task_rq_rr_t, rq);
  Z_LOG_WARN("TODO: Free pending tasks of RQ-RR %p", rr);
}

static void __task_rq_rr_add (z_task_rq_t *self, z_vtask_t *vtask) {
  z_task_rq_rr_t *rr = z_container_of(self, z_task_rq_rr_t, rq);
  if (vtask->flags.type == Z_VTASK_TYPE_RQ) {
    rr->prio_sum += z_vtask_rq_prio(vtask);
    z_vtask_rq_quantum(vtask) = __rq_quantum(rr, z_vtask_rq_prio(vtask));
  }
  z_vtask_queue_push(&(rr->queue), vtask);
}

static z_vtask_t *__task_rq_rr_fetch (z_task_rq_t *self) {
  z_task_rq_rr_t *rr = z_container_of(self, z_task_rq_rr_t, rq);
  z_vtask_t *vtask = rr->queue.head;
  if (vtask->flags.type == Z_VTASK_TYPE_RQ) {
    if (--z_vtask_rq_quantum(vtask) == 0) {
      /* TODO: Add move to tail */
      vtask = z_vtask_queue_pop(&(rr->queue));
      z_vtask_rq_quantum(vtask) = __rq_quantum(rr, z_vtask_rq_prio(vtask));
      z_vtask_queue_push(&(rr->queue), vtask);
      self->size += 1;
    }
    return(vtask);
  }
  return(z_vtask_queue_pop(&(rr->queue)));
}

const z_vtable_task_rq_t z_task_rq_rr = {
  .open   = __task_rq_rr_open,
  .close  = __task_rq_rr_close,
  .add    = __task_rq_rr_add,
  .readd  = __task_rq_rr_add,
  .fetch  = __task_rq_rr_fetch,
  .fini   = NULL,
};
