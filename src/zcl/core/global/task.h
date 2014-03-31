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

#ifndef _Z_TASK_H_
#define _Z_TASK_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/locking.h>
#include <zcl/macros.h>
#include <zcl/opaque.h>
#include <zcl/dlink.h>
#include <zcl/tree.h>
#include <zcl/bits.h>

#define Z_TASK(x)                 Z_CAST(z_task_t, x)

Z_TYPEDEF_STRUCT(z_vtable_task_rq)
Z_TYPEDEF_STRUCT(z_task_rq_fair)
Z_TYPEDEF_STRUCT(z_task_rq_fifo)
Z_TYPEDEF_STRUCT(z_task_rwcsem)
Z_TYPEDEF_STRUCT(z_task_sched)
Z_TYPEDEF_STRUCT(z_task_queue)
Z_TYPEDEF_STRUCT(z_task_tree)
Z_TYPEDEF_STRUCT(z_task_rq)
Z_TYPEDEF_STRUCT(z_task)

typedef void (*z_task_func_t) (z_task_t *task);

struct z_task {
  z_tree_node_t sys_node;
  z_tree_node_t usr_node;
  // sys_node.uflags8  : state
  // sys_node.uflags16 : vtime_hi
  // sys_node.uflags32 : seqid_hi
  // usr_node.uflags8  : flags
  // usr_node.uflags16 : vtime_lo
  // usr_node.uflags32 : seqid_lo
  z_task_func_t func;
};

struct z_task_queue {
  z_task_t *head;
};

struct z_task_tree {
  z_tree_node_t *root;
};

struct z_task_rq {
  z_dlink_node_t link;

  uint8_t  prio;           /* rq priority */
  uint8_t  quantum;        /* rq avail quantum */
  uint16_t pad;
  uint32_t size;

  const z_vtable_task_rq_t *vtable;
};

struct z_task_rq_fifo {
  z_task_rq_t    rq;
  z_task_tree_t  pending;
  z_task_queue_t queue;
  uint64_t       seqid;
};

struct z_task_rq_fair {
  z_task_rq_t    rq;
  z_tree_node_t *queue;
  uint64_t       seqid;
};

struct z_vtable_task_rq {
  void       (*open)  (z_task_rq_t *self);
  void       (*close) (z_task_rq_t *self);
  void       (*add)   (z_task_rq_t *self, z_task_t *task);
  z_task_t * (*fetch) (z_task_rq_t *self);
};

struct z_task_sched {
  z_dlink_node_t rqs;    /* Run queues */
  uint16_t prio_sum;     /* Sum of the rqs priorities */
  uint8_t  prio_shl;     /* Priority shift */
  uint8_t  quantum;      /* rq default quantum */
  uint8_t  rq_size;
  uint8_t  pad[4];
};

struct z_task_rwcsem {
  z_rwcsem_t   lock;                      /* Object RWC-Lock */
  z_spinlock_t wlock;                     /* Object wait-queue lock */
  z_task_queue_t readq;                   /* Object task-read wait queue */
  z_task_queue_t writeq;                  /* Object task-write wait queue */
  z_task_queue_t commitq;                 /* Object task-commit wait queue */
  z_task_queue_t lockq;                   /* Object task-write wait queue */
};

extern const z_vtable_task_rq_t z_task_rq_fifo;
extern const z_vtable_task_rq_t z_task_rq_fair;

const z_tree_info_t z_task_seqid_tree_info;
const z_tree_info_t z_task_vtime_tree_info;

#define z_task_data(self, type)       ((type *)((self)->data))
#define z_task_exec(self)             (self)->func(self)
#define z_task_state(self)            (self)->sys_node.uflags8
#define z_task_set_state(self, x)     (self)->sys_node.uflags8 = (x)
#define z_task_flags(self)            (self)->usr_node.uflags8
#define z_task_set_flags(self, x)     (self)->usr_node.uflags8 = (x)

#define z_task_seqid(self)                                                \
  (z_shl32((self)->sys_node.uflags32) + (self)->usr_node.uflags32)

#define z_task_set_seqid(self, seqid)                                     \
  do {                                                                    \
    uint64_t task_id = (seqid);                                           \
    (self)->sys_node.uflags32 = (task_id) >> 32;                          \
    (self)->usr_node.uflags32 = (task_id) & 0xffffffff;                   \
  } while (0)

#define z_task_vtime(self)                                                \
  (z_shl16((self)->sys_node.uflags16) + (self)->usr_node.uflags16)

#define z_task_set_vtime(self, value)                                     \
  do {                                                                    \
    uint32_t vtime = (value);                                             \
    (self)->sys_node.uflags16 = (vtime) >> 16;                            \
    (self)->usr_node.uflags16 = (vtime) & 0xffff;                         \
  } while (0)

#define z_task_reset(self, task_func)                                     \
  do {                                                                    \
    (self)->sys_node.uflags8  = 0;                                        \
    (self)->sys_node.uflags16 = 0;                                        \
    (self)->sys_node.uflags32 = 0;                                        \
    (self)->usr_node.uflags8  = 0;                                        \
    (self)->usr_node.uflags16 = 0;                                        \
    (self)->usr_node.uflags32 = 0;                                        \
    (self)->func = task_func;                                             \
  } while (0)

#define z_task_rq_close(self)           (self)->vtable->close(self)
#define z_task_rq_add(self, task)       (self)->vtable->add(self, task)
#define z_task_rq_fetch(self)           (self)->vtable->fetch(self)

void          z_task_rq_open          (z_task_rq_t *self,
                                       const z_vtable_task_rq_t *vtable,
                                       uint8_t priority);

void          z_task_sched_open       (z_task_sched_t *self);
void          z_task_sched_close      (z_task_sched_t *self);
void          z_task_sched_add_rq     (z_task_sched_t *self, z_task_rq_t *rq);
void          z_task_sched_remove_rq  (z_task_sched_t *self, z_task_rq_t *rq);
z_task_rq_t * z_task_sched_get_rq     (z_task_sched_t *self);

void          z_task_queue_open       (z_task_queue_t *self);
void          z_task_queue_close      (z_task_queue_t *self);
void          z_task_queue_push       (z_task_queue_t *self, z_task_t *task);
z_task_t *    z_task_queue_pop        (z_task_queue_t *self);
z_task_t *    z_task_queue_drain      (z_task_queue_t *self);

void          z_task_tree_open        (z_task_tree_t *self);
void          z_task_tree_close       (z_task_tree_t *self);
void          z_task_tree_push        (z_task_tree_t *self, z_task_t *task);
z_task_t *    z_task_tree_pop         (z_task_tree_t *self);

int           z_task_rwcsem_open      (z_task_rwcsem_t *self);
void          z_task_rwcsem_close     (z_task_rwcsem_t *self);
int           z_task_rwcsem_acquire   (z_task_rwcsem_t *self,
                                       z_rwcsem_op_t operation_type,
                                       z_task_t *task);
void          z_task_rwcsem_release   (z_task_rwcsem_t *self,
                                       z_rwcsem_op_t operation_type,
                                       z_task_t *task,
                                       int is_complete);

__Z_END_DECLS__

#endif /* _Z_TASK_H_ */
