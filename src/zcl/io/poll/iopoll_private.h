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


#ifndef _Z_IOPOLL_PRIVATE_H_
#define _Z_IOPOLL_PRIVATE_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/histogram.h>
#include <zcl/ticket.h>
#include <zcl/macros.h>
#include <zcl/thread.h>
#include <zcl/opaque.h>
#include <zcl/system.h>
#include <zcl/iopoll.h>

Z_TYPEDEF_STRUCT(z_vtable_iopoll)
Z_TYPEDEF_STRUCT(z_iopoll_engine)
Z_TYPEDEF_STRUCT(z_iopoll)

#define Z_IOPOLL_ENGINE(x)          Z_CAST(z_iopoll_engine_t, x)

#define Z_IOPOLL_WATCH              0
#define Z_IOPOLL_READ               1
#define Z_IOPOLL_WRITE              2
#define Z_IOPOLL_HANG               3
#define Z_IOPOLL_DATA               4

#define Z_IOPOLL_WATCHED            (1U << Z_IOPOLL_WATCH)
#define Z_IOPOLL_READABLE           (1U << Z_IOPOLL_READ)
#define Z_IOPOLL_WRITABLE           (1U << Z_IOPOLL_WRITE)
#define Z_IOPOLL_HANGUP             (1U << Z_IOPOLL_HANG)
#define Z_IOPOLL_HAS_DATA           (1U << Z_IOPOLL_DATA)

struct z_vtable_iopoll {
  int     (*open)     (z_iopoll_engine_t *engine);
  void    (*close)    (z_iopoll_engine_t *engine);
  int     (*insert)   (z_iopoll_engine_t *engine,
                       z_iopoll_entity_t *entry);
  int     (*remove)   (z_iopoll_engine_t *engine,
                       z_iopoll_entity_t *entry);
  void    (*poll)     (z_iopoll_engine_t *engine);
};


struct z_iopoll_engine {
  z_iopoll_stats_t stats;

  z_opaque_t  data;
  z_thread_t  thread;
  uint8_t     pad[Z_CACHELINE_PAD(sizeof(z_thread_t) + sizeof(z_opaque_t))];
};

#ifdef Z_IOPOLL_HAS_EPOLL
  extern const z_vtable_iopoll_t z_iopoll_epoll;
#endif /* Z_IOPOLL_HAS_EPOLL */

#ifdef Z_IOPOLL_HAS_KQUEUE
  extern const z_vtable_iopoll_t z_iopoll_kqueue;
#endif /* Z_IOPOLL_HAS_KQUEUE */

void z_iopoll_process (z_iopoll_engine_t *engine,
                       z_iopoll_entity_t *entity,
                       uint32_t events);

#define z_iopoll_stats_add_events(engine, nevents, wait_time)               \
  do {                                                                      \
    z_histogram_t *iowait = &((engine)->stats.iowait);                      \
    z_histogram_add(iowait, wait_time);                                     \
    iowait->max_events = z_max(iowait->max_events, nevents);                \
  } while (0)

const uint8_t *z_iopoll_is_running (void);

__Z_END_DECLS__

#endif /* !_Z_IOPOLL_PRIVATE_H_ */
