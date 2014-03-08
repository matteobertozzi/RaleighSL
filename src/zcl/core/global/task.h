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
#include <zcl/tree.h>

#define Z_TASK(x)                 Z_CAST(z_task_t, x)

Z_TYPEDEF_STRUCT(z_task_rwcsem)
Z_TYPEDEF_STRUCT(z_task_queue)
Z_TYPEDEF_STRUCT(z_task_tree)
Z_TYPEDEF_STRUCT(z_task)

typedef void (*z_task_func_t) (z_task_t *task);

struct z_task {
  z_tree_node_t __node__;

  uint64_t seqid;
  z_task_func_t func;

  uint32_t state;
  uint32_t flags;

  z_opaque_t object;
  void *context;
  void *udata;

  z_opaque_t args[5];
};

struct z_task_queue {
  z_task_t *head;
};

struct z_task_tree {
  z_tree_node_t *root;
};

struct z_task_rwcsem {
  z_rwcsem_t   lock;                      /* Object RWC-Lock */
  z_spinlock_t wlock;                     /* Object wait-queue lock */
  z_task_queue_t readq;                   /* Object task-read wait queue */
  z_task_queue_t writeq;                  /* Object task-write wait queue */
  z_task_queue_t commitq;                 /* Object task-commit wait queue */
  z_task_queue_t lockq;                   /* Object task-write wait queue */
};

z_task_t *z_task_alloc          (z_task_func_t func);
void      z_task_free           (z_task_t *task);
void      z_task_reset_time     (z_task_t *task);
#define   z_task_exec(task)     Z_TASK(task)->func(task)

void      z_task_queue_open     (z_task_queue_t *self);
void      z_task_queue_close    (z_task_queue_t *self);
void      z_task_queue_push     (z_task_queue_t *self, z_task_t *task);
z_task_t *z_task_queue_pop      (z_task_queue_t *self);

void      z_task_tree_open      (z_task_tree_t *self);
void      z_task_tree_close     (z_task_tree_t *self);
void      z_task_tree_push      (z_task_tree_t *self, z_task_t *task);
z_task_t *z_task_tree_pop       (z_task_tree_t *self);

int       z_task_rwcsem_open    (z_task_rwcsem_t *self);
void      z_task_rwcsem_close   (z_task_rwcsem_t *self);
int       z_task_rwcsem_acquire (z_task_rwcsem_t *self,
                                 z_rwcsem_op_t operation_type,
                                 z_task_t *task);
void      z_task_rwcsem_release (z_task_rwcsem_t *self,
                                 z_rwcsem_op_t operation_type,
                                 z_task_t *task,
                                 int is_complete);

__Z_END_DECLS__

#endif /* _Z_TASK_H_ */
