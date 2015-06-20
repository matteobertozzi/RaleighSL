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

typedef struct z_event_loop z_event_loop_t;

struct z_event_loop {
  /* iopoll-engine */
  z_iopoll_engine_t iopoll;
/* (16 * 64) = 1024 */

  /* channels */
  z_iopoll_entity_t   entity_event;
  z_iopoll_entity_t   entity_timeout;
  uint8_t __pad1[32];
/* (1 * 64) = 64 */

  /* memory */
  z_memory_t    memory;
  z_allocator_t allocator;
/* (5 * 64) = 320 */

  /* internals */
  uint64_t      nevents;
  int           core;
  int           is_detached;
  int           is_running;
  int           __pad2;
  z_thread_t    thread;
  uint8_t       __pad3[Z_CACHELINE_PAD_FIELDS(uint64_t, int, int, int, int, z_thread_t)];
} __attribute__((packed));

#define z_eloop_channel_notify(eloop)                                         \
  z_iopoll_engine_notify(&((eloop)->iopoll), &((eloop)->entity_event))

int   z_event_loop_open   (z_event_loop_t *self, uint32_t core);
void  z_event_loop_close  (z_event_loop_t *self);
void  z_event_loop_dump   (z_event_loop_t *self, FILE *stream);
int   z_event_loop_start  (z_event_loop_t *self, int start_thread);
int   z_event_loop_stop   (z_event_loop_t *self);
void  z_event_loop_wait   (z_event_loop_t *self);

__Z_END_DECLS__

#endif /* _Z_IOPOLL_H_ */
