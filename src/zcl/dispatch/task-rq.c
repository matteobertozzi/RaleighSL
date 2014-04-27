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
#include <zcl/debug.h>

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
 *  PUBLIC Task-RQ methods
 */
void z_task_rq_open (z_task_rq_t *self,
                     const z_vtable_task_rq_t *vtable,
                     uint8_t priority)
{
  z_vtask_reset(&(self->vtask), Z_VTASK_TYPE_RQ);
  self->vtask.uflags[0] = __task_rq_register(vtable);
  self->vtask.uflags[1] = priority;
  self->vtask.uflags[2] = 0;
  z_ticket_init(&(self->lock));
  self->size  = 0;
  self->seqid = 0;
  vtable->open(self);
}

void z_task_rq_close (z_task_rq_t *self) {
  const z_vtable_task_rq_t *vtable = __task_rq_vtables[self->vtask.uflags[0]];
  vtable->close(self);
}

void z_task_rq_add (z_task_rq_t *self, z_vtask_t *task) {
  const z_vtable_task_rq_t *vtable = __task_rq_vtables[self->vtask.uflags[0]];
  task->parent = &(self->vtask);
  z_ticket_acquire(&(self->lock));
  if (task->seqid == 0) {
    task->seqid = ++(self->seqid);
    vtable->add(self, task);
  } else {
    vtable->readd(self, task);
  }
  self->size += 1;
  z_ticket_release(&(self->lock));
}

z_vtask_t *z_task_rq_fetch (z_task_rq_t *self) {
  const z_vtable_task_rq_t *vtable = __task_rq_vtables[self->vtask.uflags[0]];
  z_vtask_t *task;
  z_ticket_acquire(&(self->lock));
  if (self->size > 0) {
    self->size -= 1;
    task = vtable->fetch(self);
  } else {
    task = NULL;
  }
  z_ticket_release(&(self->lock));
  return(task);
}

void z_task_rq_fini (z_task_rq_t *self) {
  const z_vtable_task_rq_t *vtable = __task_rq_vtables[self->vtask.uflags[0]];
  if (vtable->fini != NULL) {
    unsigned int size;

    z_ticket_acquire(&(self->lock));
    vtable->fini(self);
    size = self->size;
    z_ticket_release(&(self->lock));

    z_global_new_tasks_signal(size);
  }
}
