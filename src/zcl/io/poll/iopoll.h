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

#ifndef _Z_IOPOLL_H_
#define _Z_IOPOLL_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/histogram.h>
#include <zcl/macros.h>
#include <zcl/ticket.h>
#include <zcl/opaque.h>

Z_TYPEDEF_STRUCT(z_iopoll_entity_vtable)
Z_TYPEDEF_STRUCT(z_iopoll_vtable)
Z_TYPEDEF_STRUCT(z_iopoll_entity)
Z_TYPEDEF_STRUCT(z_iopoll_engine)
Z_TYPEDEF_STRUCT(z_iopoll_timer)
Z_TYPEDEF_STRUCT(z_iopoll_stats)
Z_TYPEDEF_STRUCT(z_iopoll_load)

#define Z_IOPOLL_ENTITY_FD(x)       (Z_IOPOLL_ENTITY(x)->fd)
#define Z_IOPOLL_ENTITY(x)          Z_CAST(z_iopoll_entity_t, x)

#define __Z_IOPOLL_ENTITY__         z_iopoll_entity_t __io_entity__;

#define Z_IOPOLL_READ               0
#define Z_IOPOLL_WRITE              1
#define Z_IOPOLL_TIMER              2
#define Z_IOPOLL_USER               3

#define Z_IOPOLL_WATCH              4
#define Z_IOPOLL_HANG               5
#define Z_IOPOLL_DATA               6

#define Z_IOPOLL_READABLE           (1U << Z_IOPOLL_READ)
#define Z_IOPOLL_WRITABLE           (1U << Z_IOPOLL_WRITE)
#define Z_IOPOLL_TIMEOUT            (1U << Z_IOPOLL_TIMER)
#define Z_IOPOLL_UEVENT             (1U << Z_IOPOLL_USER)

#define Z_IOPOLL_WATCHED            (1U << Z_IOPOLL_WATCH)
#define Z_IOPOLL_HANGUP             (1U << Z_IOPOLL_HANG)
#define Z_IOPOLL_HAS_DATA           (1U << Z_IOPOLL_DATA)

struct z_iopoll_entity_vtable {
  int     (*read)     (z_iopoll_entity_t *entity);
  int     (*write)    (z_iopoll_entity_t *entity);
  int     (*uevent)   (z_iopoll_entity_t *entity);
  int     (*timeout)  (z_iopoll_entity_t *entity);
  void    (*close)    (z_iopoll_entity_t *entity);
};

struct z_iopoll_vtable {
  int     (*add)      (z_iopoll_engine_t *engine,
                       z_iopoll_entity_t *entry);
  int     (*remove)   (z_iopoll_engine_t *engine,
                       z_iopoll_entity_t *entry);
  int     (*timer)    (z_iopoll_engine_t *engine,
                       z_iopoll_entity_t *entry,
                       uint32_t msec);
  int     (*uevent)   (z_iopoll_engine_t *engine,
                       z_iopoll_entity_t *entry,
                       int enable);
  int     (*notify)   (z_iopoll_engine_t *engine,
                       z_iopoll_entity_t *entry);

  int     (*open)     (z_iopoll_engine_t *engine);
  void    (*close)    (z_iopoll_engine_t *engine);
  void    (*poll)     (z_iopoll_engine_t *engine);
};

struct z_iopoll_entity {
  uint8_t    type;
  uint8_t    iflags8;
  uint8_t    ioengine;
  uint8_t    uflags8;
  uint32_t   last_write_ts;
  int        fd;
  uint32_t   __pad;
};

struct z_iopoll_load {
  uint8_t  tail;
  uint8_t  max_events;
  uint8_t  events[6];
  uint32_t idle[6];
  uint32_t active[6];
};

struct z_iopoll_stats {
  z_iopoll_load_t ioload;

  z_histogram_t   iowait;
  uint64_t        iowait_events[20];

  z_histogram_t   ioread;
  uint64_t        ioread_events[20];

  z_histogram_t   iowrite;
  uint64_t        iowrite_events[20];

  z_histogram_t   event;
  uint64_t        event_events[20];

  z_histogram_t   timeout;
  uint64_t        timeout_events[20];
};

struct z_iopoll_engine {
  z_opaque_t       data;
  z_iopoll_stats_t stats;
};

#ifdef Z_IOPOLL_HAS_EPOLL
  extern const z_iopoll_vtable_t z_iopoll_epoll;
#endif /* Z_IOPOLL_HAS_EPOLL */

#ifdef Z_IOPOLL_HAS_KQUEUE
  extern const z_iopoll_vtable_t z_iopoll_kqueue;
#endif /* Z_IOPOLL_HAS_KQUEUE */

#if defined(Z_IOPOLL_HAS_EPOLL)
  #define z_iopoll_engine           z_iopoll_epoll
#elif defined(Z_IOPOLL_HAS_KQUEUE)
  #define z_iopoll_engine           z_iopoll_kqueue
#else
  #error "IOPoll engine missing"
#endif

#ifndef Z_IOPOLL_MAX_EVENTS
  #define Z_IOPOLL_MAX_EVENTS           (128)
#endif /* !Z_IOPOLL_MAX_EVENTS */

int  z_iopoll_open  (unsigned int ncores);
void z_iopoll_close (void);
void z_iopoll_stop  (void);
void z_iopoll_wait  (void);

int  z_iopoll_add     (z_iopoll_entity_t *entity);
int  z_iopoll_remove  (z_iopoll_entity_t *entity);
int  z_iopoll_timer   (z_iopoll_entity_t *entity, uint32_t msec);
int  z_iopoll_uevent  (z_iopoll_entity_t *entity, int enable);
int  z_iopoll_notify  (z_iopoll_entity_t *entity);
int  z_iopoll_poll    (int detached, const int *is_looping);

void z_iopoll_process       (z_iopoll_engine_t *engine,
                             z_iopoll_entity_t *entity,
                             uint32_t events);

int  z_iopoll_engine_open   (z_iopoll_engine_t *engine);
void z_iopoll_engine_close  (z_iopoll_engine_t *engine);
void z_iopoll_engine_dump   (z_iopoll_engine_t *engine);

void z_iopoll_entity_open   (z_iopoll_entity_t *entity,
                             const z_iopoll_entity_vtable_t *vtable,
                             int fd);
void z_iopoll_entity_close  (z_iopoll_entity_t *entity, int defer);

void z_iopoll_set_data_available (z_iopoll_entity_t *entity, int has_data);
void z_iopoll_set_writable       (z_iopoll_entity_t *entity, int writable);
void z_iopoll_add_latencies      (z_iopoll_entity_t *entity,
                                  uint64_t req_latency,
                                  uint64_t resp_latency);

void z_iopoll_timer_open    (z_iopoll_entity_t *entity,
                             const z_iopoll_entity_vtable_t *vtable,
                             int oid);
void z_iopoll_uevent_open   (z_iopoll_entity_t *entity,
                             const z_iopoll_entity_vtable_t *vtable,
                             int oid);

float z_iopoll_engine_load      (z_iopoll_engine_t *engine);
void  z_iopoll_stats_add_events (z_iopoll_engine_t *engine,
                                 int nevents,
                                 uint64_t idle_usec);

#define z_iopoll_stats_add_active(self, usec)                                  \
  (self)->stats.ioload.active[(self)->stats.ioload.tail] += (usec) & 0xffffffff

__Z_END_DECLS__

#endif /* _Z_IOPOLL_H_ */
