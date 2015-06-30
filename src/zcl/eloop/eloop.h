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

#ifndef _Z_ELOOP_H_
#define _Z_ELOOP_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/allocator.h>
#include <zcl/system.h>
#include <zcl/memory.h>
#include <zcl/thread.h>
#include <zcl/iopoll.h>

typedef struct z_event_loop_vtable z_event_loop_vtable_t;
typedef struct z_event_loop z_event_loop_t;

struct z_event_loop {
  /* iopoll-engine */
  z_iopoll_engine_t iopoll;
/* (16 * 64) = 1024 */

  /* memory */
  z_memory_t    memory;
  z_allocator_t allocator;
/* (5 * 64) = 320 */

  /* internals */
  const z_event_loop_vtable_t *vtable;
  z_opaque_t        udata;
  z_iopoll_entity_t entity_event;
  z_iopoll_entity_t entity_timeout;

  int           core;
  uint8_t       is_detached;
  uint8_t       is_running;
  uint8_t       __pad1[2];
  z_thread_t    thread;
};

struct z_event_loop_vtable {
  int (*exec)    (z_event_loop_t *eloop);
  int (*timeout) (z_event_loop_t *eloop);
  int (*event)   (z_event_loop_t *eloop);
};

extern const z_event_loop_vtable_t z_event_loop_noop_vtable;

#define z_eloop_channel_notify(eloop)                                         \
  z_iopoll_engine_notify(&((eloop)->iopoll), &((eloop)->entity_event))

#define z_eloop_channel_set_timer(eloop, msec)                                \
  z_iopoll_engine_timer(&((eloop)->iopoll), &((eloop)->entity_timeout), msec)

int   z_event_loop_open   (z_event_loop_t *self,
                           const z_event_loop_vtable_t *vtable,
                           uint32_t core);
void  z_event_loop_close  (z_event_loop_t *self);
void  z_event_loop_dump   (z_event_loop_t *self, FILE *stream);
int   z_event_loop_start  (z_event_loop_t *self, int start_thread);
int   z_event_loop_stop   (z_event_loop_t *self);
void  z_event_loop_wait   (z_event_loop_t *self);

__Z_END_DECLS__

#endif /* _Z_IOPOLL_H_ */
