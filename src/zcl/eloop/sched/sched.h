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

#ifndef _Z_SCHED_H_
#define _Z_SCHED_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/dlink.h>

typedef struct z_sched_task {
  z_dlink_node_t link;
} z_sched_task_t;

typedef struct z_sched_queue {
  z_dlink_node_t link;
  z_dlink_node_t task_list;
} z_sched_queue_t;

typedef struct sched {
  z_dlink_node_t queue_list;
} z_sched_t;

void            z_sched_init        (z_sched_t *self);
void            z_sched_suspend     (z_sched_t *self, z_sched_queue_t *queue);
void            z_sched_resume      (z_sched_t *self, z_sched_queue_t *queue);
int             z_sched_has_tasks   (const z_sched_t *self);
z_sched_task_t *z_sched_poll        (z_sched_t *self);
void z_sched_add (z_sched_t *self, z_sched_queue_t *queue, z_sched_task_t *task);

void            z_sched_queue_init  (z_sched_queue_t *self);

void            z_sched_task_init   (z_sched_task_t *self);

__Z_END_DECLS__

#endif /* !_Z_SCHED_H_ */
