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

#include <zcl/debug.h>
#include <zcl/timer.h>
#include <zcl/eloop.h>
#include <zcl/math.h>

/* ============================================================================
 *  PRIVATE Event Loop (IO-Entity)
 */
static int __event_loop_event (z_iopoll_engine_t *engine,
                               z_iopoll_entity_t *entity)
{
  z_event_loop_t *eloop = z_container_of(entity, z_event_loop_t, entity_event);
  return eloop->vtable->event(eloop);
}

static int __event_loop_timeout (z_iopoll_engine_t *engine,
                                 z_iopoll_entity_t *entity)
{
  z_event_loop_t *eloop = z_container_of(entity, z_event_loop_t, entity_timeout);
  return eloop->vtable->timeout(eloop);
}

const z_iopoll_entity_vtable_t __event_loop_vtable = {
  .read     = NULL,
  .write    = NULL,
  .uevent   = __event_loop_event,
  .timeout  = __event_loop_timeout,
  .close    = NULL,
};

static void *__event_loop_exec (void *args) {
  z_event_loop_t *eloop = Z_CAST(z_event_loop_t, args);

  z_eloop_channel_notify(eloop);
  while (eloop->is_running) {
    z_timer_t timer;

    /* process pending events */
    /* TODO: pass max events to slowdown */
    z_iopoll_engine.poll(&(eloop->iopoll));

    /* fetch messages from other cores (on event?) */

    /* execute pending tasks */
    z_timer_start(&timer);
    eloop->vtable->exec(eloop);
    z_timer_stop(&timer);
    z_iopoll_stats_add_active(&(eloop->iopoll), z_timer_micros(&timer));
  }
  z_eloop_channel_notify(eloop);
  return(NULL);
}

/* ============================================================================
 *  PUBLIC Event Loop Allocator
 */
static void *__event_loop_mmalloc (z_allocator_t *self, size_t size) {
  void *ptr;
  ptr = z_sys_alloc(size);
  if (Z_LIKELY(ptr != NULL)) {
    self->extern_alloc++;
    self->extern_used += size;
  }
  return(ptr);
}

static void __event_loop_mmfree (z_allocator_t *self, void *ptr, size_t size) {
  self->extern_used -= size;
  Z_LOG_TRACE("pool_used=%"PRIu64" extern_used=%"PRIu64,
              self->pool_used, self->extern_used);
  z_sys_free(ptr, size);
}

/* ============================================================================
 *  PUBLIC Event Loop
 */
void z_event_loop_dump (z_event_loop_t *self, FILE *stream) {
  fprintf(stream, "==================================\n");
  fprintf(stream, " ENGINE %d\n", self->core);
  fprintf(stream, "==================================\n");

  fprintf(stream, "Memory Stats\n");
  fprintf(stream, "==================================\n");
  fprintf(stream, "  pool alloc %"PRIu64"\n", self->allocator.pool_alloc);
  fprintf(stream, "  pool used  %"PRIu64"\n", self->allocator.pool_used);
  fprintf(stream, "extern alloc %"PRIu64"\n", self->allocator.extern_alloc);
  fprintf(stream, "extern used  %"PRIu64"\n", self->allocator.extern_used);
  z_memory_stats_dump(&(self->memory), stream);
  fprintf(stream, "\n");

  z_iopoll_engine_dump(&(self->iopoll), stream);
}

int z_event_loop_open (z_event_loop_t *self,
                       const z_event_loop_vtable_t *vtable,
                       uint32_t core)
{
  /* Initialize iopoll-engine */
  z_iopoll_engine_open(&(self->iopoll));

  /* Initialize event-loop entities */
  z_iopoll_uevent_open(&(self->entity_event), &__event_loop_vtable, core);
  self->entity_timeout.ioengine = core;
  z_iopoll_timer_open(&(self->entity_timeout), &__event_loop_vtable, core);
  self->entity_timeout.ioengine = core;
  z_iopoll_engine.uevent(&(self->iopoll), &(self->entity_event), 1);

  //z_iopoll_engine.timer(&(self->iopoll), &(self->entity_timeout), 1000);

  /* Initialize memory */
  self->allocator.mmalloc = __event_loop_mmalloc;
  self->allocator.mmfree  = __event_loop_mmfree;
  self->allocator.pool_alloc = 0;
  self->allocator.pool_used  = 0;
  self->allocator.extern_alloc = 0;
  self->allocator.extern_used  = 0;
  //self->allocator.mmpool = ctx->data + __ctx_mmpool_offset(ctx, core);
  self->allocator.udata  = self;
  z_memory_open(&(self->memory), &(self->allocator));

  /* Initialize internals */
  self->vtable = (vtable != NULL) ? vtable : &z_event_loop_noop_vtable;
  self->core = core;
  self->is_running = 0;
  self->is_detached = 0;
  return(0);
}

void z_event_loop_close (z_event_loop_t *self) {
  z_iopoll_engine.uevent(&(self->iopoll), &(self->entity_event), 0);
  z_iopoll_engine_close(&(self->iopoll));
  z_memory_close(&(self->memory));
}

int z_event_loop_start (z_event_loop_t *self, int start_thread) {
  /* start new thread if required */
  self->is_running = 1;
  if (start_thread) {
    if (z_thread_start(&(self->thread), __event_loop_exec, self)) {
      self->is_running = 0;
      return(1);
    }

    z_thread_bind_to_core(&(self->thread), self->core);
    self->is_detached = 1;
  } else {
    z_thread_self(&(self->thread));
    z_thread_bind_to_core(&(self->thread), self->core);

    __event_loop_exec(self);
  }
  return(0);
}

int z_event_loop_stop (z_event_loop_t *self) {
  self->is_running = 0;
  return(z_eloop_channel_notify(self));
}

void z_event_loop_wait (z_event_loop_t *self) {
  z_thread_join(&(self->thread));
}

/* ============================================================================
 *  Noop Event Loop vtable
 */
static int __noop_eloop_func (z_event_loop_t *eloop) {
  return(0);
}

const z_event_loop_vtable_t z_event_loop_noop_vtable = {
  .exec     = __noop_eloop_func,
  .event    = __noop_eloop_func,
  .timeout  = __noop_eloop_func,
};
