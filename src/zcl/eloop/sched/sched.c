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

#include <zcl/sched.h>

void z_sched_init (z_sched_t *self) {
  z_dlink_init(&(self->queue_list));
}

void z_sched_add (z_sched_t *self, z_sched_queue_t *queue, z_sched_task_t *task) {
  if (z_dlink_is_not_linked(&(queue->link))) {
    z_dlink_move_tail(&(self->queue_list), &(queue->link));
  }
  z_dlink_move_tail(&(queue->task_list), &(task->link));
}

void z_sched_suspend (z_sched_t *self, z_sched_queue_t *queue) {
  z_dlink_del(&(queue->link));
}

void z_sched_resume (z_sched_t *self, z_sched_queue_t *queue) {
  z_dlink_move(&(self->queue_list), &(queue->link));
}

int z_sched_has_tasks (const z_sched_t *self) {
  return z_dlink_is_not_empty(&(self->queue_list));
}

z_sched_task_t *z_sched_poll (z_sched_t *self) {
  z_sched_queue_t *queue;
  z_sched_task_t *task;

  queue = z_dlink_front_entry(&(self->queue_list), z_sched_queue_t, link);
  task = z_dlink_front_entry(&(queue->task_list), z_sched_task_t, link);

  z_dlink_del(&(task->link));
  if (z_dlink_is_not_empty(&(queue->task_list))) {
    // TODO consider quantum for the queue
    z_dlink_move_tail(&(self->queue_list), &(queue->link));
  } else {
    z_dlink_del(&(queue->link));
  }
  return task;
}

void z_sched_queue_init (z_sched_queue_t *self) {
  z_dlink_init(&(self->link));
  z_dlink_init(&(self->task_list));
}

void z_sched_task_init (z_sched_task_t *self) {
  z_dlink_init(&(self->link));
}
