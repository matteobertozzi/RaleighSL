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
 *  PRIVATE task-sched methods
 */
#define __task_rq_fairness(self, rq_prio)                                     \
  (((rq_prio) << (self)->prio_shl) / (self)->prio_sum)

#define __task_rq_quantum(self, rq_prio)                                      \
  ((__task_rq_fairness(self, rq_prio) << (self)->quantum) >> (self)->prio_shl)

/* ============================================================================
 *  PUBLIC task-sched methods
 */
void z_task_sched_open (z_task_sched_t *self) {
  z_dlink_init(&(self->rqs));
  self->prio_sum = 0;
  self->prio_shl = 0;
  self->quantum = 0;
  self->rq_size = 0;
}

void z_task_sched_close (z_task_sched_t *self) {
  z_task_rq_t *rq;
  z_dlink_del_for_each_entry(&(self->rqs), rq, z_task_rq_t, link, {
    z_task_rq_close(rq);
  });
}

void z_task_sched_add_rq (z_task_sched_t *self, z_task_rq_t *rq) {
  z_dlink_add_tail(&(self->rqs), &(rq->link));
  self->prio_sum += rq->prio;
  self->rq_size += 1;
  rq->quantum = 1 + __task_rq_quantum(self, rq->prio);
}

void z_task_sched_remove_rq (z_task_sched_t *self, z_task_rq_t *rq) {
  z_dlink_del(&(rq->link));
  self->prio_sum -= rq->prio;
  self->rq_size -= 1;
}

z_task_rq_t *z_task_sched_get_rq (z_task_sched_t *self) {
  int count = self->rq_size;
  z_task_rq_t *rq = NULL;

  do {
    rq = z_dlink_front_entry(&(self->rqs), z_task_rq_t, link);
    if (--(rq->quantum) == 0) {
      z_dlink_move_tail(&(self->rqs), &(rq->link));
      rq->quantum = 1 + __task_rq_quantum(self, rq->prio);
    }
  } while (rq->size == 0 && --count > 0);

  return(rq->size ? rq : NULL);
}
