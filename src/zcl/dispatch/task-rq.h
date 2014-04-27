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

#include <zcl/macros.h>
#include <zcl/ticket.h>
#include <zcl/vtask.h>

Z_TYPEDEF_STRUCT(z_vtable_task_rq)
Z_TYPEDEF_STRUCT(z_task_rq_group)
Z_TYPEDEF_STRUCT(z_task_rq_fifo)
Z_TYPEDEF_STRUCT(z_task_rq_fair)
Z_TYPEDEF_STRUCT(z_task_rq_rr)
Z_TYPEDEF_STRUCT(z_task_rq)

struct z_vtable_task_rq {
  void        (*open)   (z_task_rq_t *self);
  void        (*close)  (z_task_rq_t *self);
  void        (*add)    (z_task_rq_t *self, z_vtask_t *task);
  void        (*readd)  (z_task_rq_t *self, z_vtask_t *task);
  z_vtask_t * (*fetch)  (z_task_rq_t *self);
  void        (*fini)   (z_task_rq_t *self);
};

struct z_task_rq {
  z_vtask_t vtask;
  z_ticket_t lock;
  uint32_t   size;
  uint64_t   seqid;
};

struct z_task_rq_group {
  z_task_rq_t     rq;
  z_vtask_queue_t queue;
  z_vtask_queue_t pending;
  uint16_t        running;
  uint16_t        barrier;
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
  uint16_t       quantum;
  uint16_t       pad;
};

struct z_task_rq_rr {
  z_task_rq_t     rq;
  z_vtask_queue_t queue;
  uint32_t        prio_sum;
  uint16_t        quantum;
  uint16_t        pad;
};

#define z_task_rq_prio(self)              z_vtask_rq_prio(&((self)->vtask))
#define z_task_rq_quantum(self)           z_vtask_rq_quantum(&((self)->vtask))

#define z_vtask_rq_prio(vtask)            (vtask)->uflags[1]
#define z_vtask_rq_quantum(vtask)         (vtask)->uflags[2]

extern const z_vtable_task_rq_t z_task_rq_group;
extern const z_vtable_task_rq_t z_task_rq_fifo;
extern const z_vtable_task_rq_t z_task_rq_fair;
extern const z_vtable_task_rq_t z_task_rq_rr;

void       z_task_rq_open   (z_task_rq_t *self,
                             const z_vtable_task_rq_t *vtable,
                             uint8_t priority);
void       z_task_rq_close  (z_task_rq_t *self);
void       z_task_rq_add    (z_task_rq_t *self, z_vtask_t *task);
z_vtask_t *z_task_rq_fetch  (z_task_rq_t *self);
void       z_task_rq_fini   (z_task_rq_t *self);

void       z_task_group_open        (z_task_rq_group_t *self, uint8_t priority);
void       z_task_group_close       (z_task_rq_group_t *self);
void       z_task_group_add         (z_task_rq_group_t *self, z_vtask_t *vtask);
void       z_task_group_add_barrier (z_task_rq_group_t *self, z_vtask_t *vtask);
void       z_task_group_wait        (z_task_rq_group_t *self);

__Z_END_DECLS__

#endif /* _Z_TASK_RQ_H_ */
