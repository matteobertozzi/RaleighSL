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
 *  PRIVATE Fair task-rq methods
 */
static void __task_rq_fair_open (z_task_rq_t *self) {
  z_task_rq_fair_t *fair = z_container_of(self, z_task_rq_fair_t, rq);
  z_vtask_tree_init(&(fair->queue));
  fair->prio_sum = 1;
  fair->quantum = 0;
}

static void __task_rq_fair_close (z_task_rq_t *self) {
  z_task_rq_fair_t *fair = z_container_of(self, z_task_rq_fair_t, rq);
  z_vtask_tree_cancel(&(fair->queue));
}

static void __task_rq_fair_add (z_task_rq_t *self, z_vtask_t *vtask) {
  z_task_rq_fair_t *fair = z_container_of(self, z_task_rq_fair_t, rq);
  if (vtask->flags.type == Z_VTASK_TYPE_RQ) {
    fair->prio_sum += vtask->priority;
    vtask->flags.quantum = z_task_rq_quantum(fair, vtask->priority);
  }
  z_vtask_tree_push(&(fair->queue), vtask);
}

static void __task_rq_fair_remove (z_task_rq_t *self, z_vtask_t *vtask) {
  z_task_rq_fair_t *fair = z_container_of(self, z_task_rq_fair_t, rq);
  if (fair->queue.root != NULL) {
    if (vtask->flags.type == Z_VTASK_TYPE_RQ) {
      fair->prio_sum -= vtask->priority;
    }
    z_vtask_tree_remove(&(fair->queue), vtask);
  }
}

static z_vtask_t *__task_rq_fair_fetch (z_task_rq_t *self) {
  z_task_rq_fair_t *fair = z_container_of(self, z_task_rq_fair_t, rq);
  z_vtask_t *vtask = fair->queue.min_node;
  if (vtask->flags.type == Z_VTASK_TYPE_RQ) {
    if (--vtask->flags.quantum == 0) {
      /* TODO: Add move to tail */
      vtask = z_vtask_tree_pop(&(fair->queue));
      vtask->flags.quantum = z_task_rq_quantum(fair, vtask->priority);
      vtask->vtime += vtask->priority;
      z_vtask_tree_push(&(fair->queue), vtask);
    }
    self->size += 1;
    return(vtask);
  }
  return(z_vtask_tree_pop(&(fair->queue)));
}

const z_vtable_task_rq_t z_task_rq_fair = {
  .open   = __task_rq_fair_open,
  .close  = __task_rq_fair_close,
  .add    = __task_rq_fair_add,
  .readd  = __task_rq_fair_add,
  .remove = __task_rq_fair_remove,
  .fetch  = __task_rq_fair_fetch,
  .fini   = NULL,
};
