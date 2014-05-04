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

#ifndef _Z_TASK_RQ_H_
#define _Z_TASK_RQ_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/histogram.h>
#include <zcl/macros.h>
#include <zcl/ticket.h>
#include <zcl/vtask.h>

Z_TYPEDEF_STRUCT(z_vtable_task_rq)
Z_TYPEDEF_STRUCT(z_task_rq_stats)
Z_TYPEDEF_STRUCT(z_task_rq_group)
Z_TYPEDEF_STRUCT(z_task_rq_fifo)
Z_TYPEDEF_STRUCT(z_task_rq_fair)
Z_TYPEDEF_STRUCT(z_task_rq_rr)
Z_TYPEDEF_STRUCT(z_task_rq)

struct z_task_rq_stats {
  z_histogram_t rq_time;
  uint64_t rq_time_events[19];

  z_histogram_t task_exec;
  uint64_t task_exec_events[19];

  z_histogram_t rq_wait;
  uint64_t rq_wait_events[19];
};

struct z_vtable_task_rq {
  z_vtask_t * (*fetch)  (z_task_rq_t *self);
  void        (*add)    (z_task_rq_t *self, z_vtask_t *task);
  void        (*readd)  (z_task_rq_t *self, z_vtask_t *task);
  void        (*remove) (z_task_rq_t *self, z_vtask_t *task);
  void        (*fini)   (z_task_rq_t *self);
  void        (*open)   (z_task_rq_t *self);
  void        (*close)  (z_task_rq_t *self);
};

struct z_task_rq {
  z_vtask_t  vtask;
  z_ticket_t lock;
  uint16_t   size;
  uint16_t   refs;
  uint64_t   seqid;
};

struct z_task_rq_group {
  z_task_rq_t     rq;
  z_vtask_queue_t queue;
  z_vtask_queue_t pending;
  uint16_t        running;
  uint16_t        barrier;
  uint16_t        waiting;
  uint16_t        has_barrier;
};

struct z_task_rq_fifo {
  z_task_rq_t     rq;
  z_vtask_queue_t queue;
  z_vtask_tree_t  pending;
};

struct z_task_rq_fair {
  z_task_rq_t    rq;
  z_vtask_tree_t queue;
  uint32_t       prio_sum;
  uint8_t        quantum;
  uint8_t        _pad[3];
};

struct z_task_rq_rr {
  z_task_rq_t     rq;
  z_vtask_queue_t queue;
  uint32_t        prio_sum;
  uint8_t         quantum;
  uint8_t         _pad[3];
};

#define z_task_rq_parent(self)                                                \
  z_container_of((self)->vtask.parent, z_task_rq_t, vtask)

#define z_task_rq_fairness(self, rq_prio)                                     \
  (((rq_prio) << 8) / (self)->prio_sum)

#define z_task_rq_quantum(self, rq_prio)                                      \
  z_min(1 + ((z_task_rq_fairness(self, rq_prio) << (self)->quantum) >> 8), 7)

#define z_task_rq_attach(self, parent_rq)          \
  (self)->rq.vtask.parent = &((parent_rq)->vtask);

extern const z_vtable_task_rq_t z_task_rq_group;
extern const z_vtable_task_rq_t z_task_rq_fifo;
extern const z_vtable_task_rq_t z_task_rq_fair;
extern const z_vtable_task_rq_t z_task_rq_rr;

void       z_task_rq_stats_init (z_task_rq_stats_t *self);

void       z_task_rq_open    (z_task_rq_t *self,
                              const z_vtable_task_rq_t *vtable,
                              uint8_t priority);
void       z_task_rq_close   (z_task_rq_t *self);
void       z_task_rq_release (z_task_rq_t *self);
void       z_task_rq_add     (z_task_rq_t *self, z_vtask_t *task);
void       z_task_rq_remove  (z_task_rq_t *self, z_vtask_t *task);
z_vtask_t *z_task_rq_fetch   (z_task_rq_t *self);
void       z_task_rq_fini    (z_task_rq_t *self);

void       z_task_group_open        (z_task_rq_group_t *self, uint8_t priority);
void       z_task_group_close       (z_task_rq_group_t *self);
void       z_task_group_add         (z_task_rq_group_t *self, z_vtask_t *vtask);
void       z_task_group_add_barrier (z_task_rq_group_t *self, z_vtask_t *vtask);
void       z_task_group_wait        (z_task_rq_group_t *self);

__Z_END_DECLS__

#endif /* !_Z_TASK_RQ_H_ */
