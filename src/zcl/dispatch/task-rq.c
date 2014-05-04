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
#include <zcl/global.h>
#include <zcl/system.h>
#include <zcl/atomic.h>
#include <zcl/debug.h>
#include <zcl/time.h>

/* ============================================================================
 *  PRIVATE Task-RQ macros
 */
#define __rq_from_vtask(self)                                        \
  z_container_of(self, z_task_rq_t, vtask)

#define __rq_add_to_parent_if_not_empty(self, not_empty)             \
  if ((not_empty) && (self)->vtask.parent) {                         \
    z_task_rq_add(z_task_rq_parent(self), &((self)->vtask));         \
  }

#define __rq_add_to_parent(self)                                     \
  if ((self)->vtask.parent) {                                        \
    z_task_rq_add(z_task_rq_parent(self), &((self)->vtask));         \
  }

#define __rq_remove_from_parent_if_empty(self, empty)                \
  if ((empty) && (self)->vtask.parent) {                             \
    z_task_rq_remove(z_task_rq_parent(self), &((self)->vtask));      \
  }

#define __rq_remove_from_parent(self)                                \
  if ((self)->vtask.parent) {                                        \
    z_task_rq_remove(z_task_rq_parent(self), &((self)->vtask));      \
  }

/* ============================================================================
 *  PRIVATE Task-RQ registration
 */
static const z_vtable_task_rq_t *__task_rq_vtables[] = {
  /*  0- 7 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  /*  8-15 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  /* 16-23 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  /* 24-31 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};

static uint8_t __task_rq_register (const z_vtable_task_rq_t *vtable) {
  uint8_t i;
  for (i = 0; i < z_fix_array_size(__task_rq_vtables); ++i) {
    if (__task_rq_vtables[i] == vtable || __task_rq_vtables[i] == NULL) {
      __task_rq_vtables[i] = vtable;
      return(i);
    }
  }
  Z_LOG_FATAL("Too many task-rq registered: %d", i);
  return(0xff);
}

/* ============================================================================
 *  PUBLIC Task-RQ stats methods
 */
#define __STATS_HISTO_NBOUNDS       z_fix_array_size(__STATS_HISTO_BOUNDS)
static const uint64_t __STATS_HISTO_BOUNDS[19] = {
  Z_TIME_USEC(5),   Z_TIME_USEC(10),  Z_TIME_USEC(25),  Z_TIME_USEC(75),
  Z_TIME_USEC(250), Z_TIME_USEC(500), Z_TIME_USEC(750), Z_TIME_MSEC(1),
  Z_TIME_MSEC(5),   Z_TIME_MSEC(10),  Z_TIME_MSEC(25),  Z_TIME_MSEC(75),
  Z_TIME_MSEC(250), Z_TIME_MSEC(500), Z_TIME_MSEC(750), Z_TIME_SEC(1),
  Z_TIME_SEC(2),    Z_TIME_SEC(5),    0xffffffffffffffffll,
};

void z_task_rq_stats_init (z_task_rq_stats_t *self) {
  z_histogram_init(&(self->rq_time),  __STATS_HISTO_BOUNDS,
                   self->rq_time_events,  __STATS_HISTO_NBOUNDS);
  z_histogram_init(&(self->task_exec),  __STATS_HISTO_BOUNDS,
                   self->task_exec_events,  __STATS_HISTO_NBOUNDS);
  z_histogram_init(&(self->rq_wait),  __STATS_HISTO_BOUNDS,
                   self->rq_wait_events,  __STATS_HISTO_NBOUNDS);
}

/* ============================================================================
 *  PUBLIC Task-RQ methods
 */
void z_task_rq_open (z_task_rq_t *self,
                     const z_vtable_task_rq_t *vtable,
                     uint8_t priority)
{
  z_vtask_reset(&(self->vtask), Z_VTASK_TYPE_RQ);
  self->vtask.link_flags.type = __task_rq_register(vtable);
  self->vtask.flags.quantum = 1;
  self->vtask.priority = priority;
  z_ticket_init(&(self->lock));
  self->size  = 0;
  self->refs  = 1;
  self->seqid = 0;
  vtable->open(self);
}

void z_task_rq_close (z_task_rq_t *self) {
  const z_vtable_task_rq_t *vtable = __task_rq_vtables[self->vtask.link_flags.type];
  z_ticket_acquire(&(self->lock));
  __rq_remove_from_parent(self);
  vtable->close(self);
  z_ticket_release(&(self->lock));

  if (z_atomic_dec(&(self->refs)) > 0) {
    while (self->refs > 0) {
      z_system_cpu_relax();
    }
  }
}

void z_task_rq_add (z_task_rq_t *self, z_vtask_t *task) {
  const z_vtable_task_rq_t *vtable = __task_rq_vtables[self->vtask.link_flags.type];
  int notify;

  task->parent = &(self->vtask);
  Z_ASSERT(task->parent != task, "Can't add a RQ to itself");
  z_ticket_acquire(&(self->lock));
  if (!task->link_flags.attached) {
    if (task->flags.type == Z_VTASK_TYPE_RQ) {
      z_atomic_inc(&(__rq_from_vtask(task)->refs));
    }
    if (task->seqid == 0) {
      task->seqid = ++(self->seqid);
      vtable->add(self, task);
    } else {
      vtable->readd(self, task);
    }
    self->size += 1;
  }
  Z_ASSERT(task->link_flags.attached, "Task should be attached %p", task);
  __rq_add_to_parent_if_not_empty(self, (notify = (self->size == 1)));
  z_ticket_release(&(self->lock));

  if (notify && self->vtask.parent == NULL) {
    z_global_new_task_signal(self);
  }
}

void z_task_rq_remove (z_task_rq_t *self, z_vtask_t *task) {
  const z_vtable_task_rq_t *vtable = __task_rq_vtables[self->vtask.link_flags.type];
  int do_remove;

  z_ticket_acquire(&(self->lock));
  if ((do_remove = task->link_flags.attached)) {
    Z_ASSERT(self->size > 0, "No items in the rq %p size=%u task-seqid=%"PRIu64,
             self, self->size, task->seqid);
    vtable->remove(self, task);
#if 1
    --self->size;
#else
    __rq_remove_from_parent_if_empty(self, --self->size == 0);
#endif
  }
  z_ticket_release(&(self->lock));
  Z_ASSERT(!task->link_flags.attached, "Task should be detached %p", task);

  if (do_remove && task->flags.type == Z_VTASK_TYPE_RQ) {
    z_atomic_dec(&(__rq_from_vtask(task)->refs));
  }
}

z_vtask_t *z_task_rq_fetch (z_task_rq_t *self) {
  const z_vtable_task_rq_t *vtable = __task_rq_vtables[self->vtask.link_flags.type];
  z_vtask_t *task;
  z_ticket_acquire(&(self->lock));
  if (self->size > 0) {
    task = vtable->fetch(self);
    if (task == NULL) {
      __rq_remove_from_parent(self);
    } else {
      if (task->flags.type == Z_VTASK_TYPE_RQ) {
        z_atomic_inc(&(__rq_from_vtask(task)->refs));
      }
#if 1
      --self->size;
#else
      __rq_remove_from_parent_if_empty(self, --self->size == 0);
#endif
    }
  } else {
    __rq_remove_from_parent(self);
    task = NULL;
  }
  z_ticket_release(&(self->lock));
  return(task);
}

void z_task_rq_fini (z_task_rq_t *self) {
  const z_vtable_task_rq_t *vtable = __task_rq_vtables[self->vtask.link_flags.type];
  if (vtable->fini != NULL) {
    unsigned int old_size;
    unsigned int size;

    z_ticket_acquire(&(self->lock));
    old_size = self->size;
    vtable->fini(self);
    size = self->size;
    __rq_add_to_parent_if_not_empty(self, (size - old_size) > 0);
    z_ticket_release(&(self->lock));
  }
}
