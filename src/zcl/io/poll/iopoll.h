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
#include <zcl/opaque.h>

#define Z_IOPOLL_ENGINES            1
#if (Z_IOPOLL_ENGINES > 1)
  #include <zcl/threading.h>
#endif /* Z_IOPOLL_ENGINES > 1 */

#define Z_IOPOLL_WATCH              0
#define Z_IOPOLL_READ               1
#define Z_IOPOLL_WRITE              2
#define Z_IOPOLL_HANG               3

#define Z_IOPOLL_WATCHED            (1U << Z_IOPOLL_WATCH)
#define Z_IOPOLL_READABLE           (1U << Z_IOPOLL_READ)
#define Z_IOPOLL_WRITABLE           (1U << Z_IOPOLL_WRITE)
#define Z_IOPOLL_HANGUP             (1U << Z_IOPOLL_HANG)

#define Z_IOPOLL_ENTITY_FD(x)       (Z_IOPOLL_ENTITY(x)->fd)
#define Z_IOPOLL_ENTITY(x)          Z_CAST(z_iopoll_entity_t, x)
#define Z_IOPOLL(x)                 Z_CAST(z_iopoll_t, x)

#define __Z_IOPOLL_ENTITY__         z_iopoll_entity_t __io_entity__;

Z_TYPEDEF_STRUCT(z_vtable_iopoll_entity)
Z_TYPEDEF_STRUCT(z_vtable_iopoll)
Z_TYPEDEF_STRUCT(z_iopoll_entity)
Z_TYPEDEF_STRUCT(z_iopoll_engine)
Z_TYPEDEF_STRUCT(z_iopoll_stats)
Z_TYPEDEF_STRUCT(z_iopoll)

struct z_vtable_iopoll_entity {
  int     (*read)     (z_iopoll_entity_t *entity);
  int     (*write)    (z_iopoll_entity_t *entity);
  void    (*close)    (z_iopoll_entity_t *entity);
};

struct z_vtable_iopoll {
  int     (*open)     (z_iopoll_t *self,
                       z_iopoll_engine_t *engine);
  void    (*close)    (z_iopoll_t *self,
                       z_iopoll_engine_t *engine);
  int     (*insert)   (z_iopoll_t *self,
                       z_iopoll_engine_t *engine,
                       z_iopoll_entity_t *entry);
  int     (*remove)   (z_iopoll_t *self,
                       z_iopoll_engine_t *engine,
                       z_iopoll_entity_t *entry);
  void    (*poll)     (z_iopoll_t *self,
                       z_iopoll_engine_t *engine);
};

struct z_iopoll_entity {
  const z_vtable_iopoll_entity_t *vtable;
  unsigned int flags;
  int fd;
};

struct z_iopoll_stats {
  z_histogram_t iowait;
  z_histogram_t ioread;
  z_histogram_t iowrite;
  uint32_t max_events;
};

struct z_iopoll_engine {
  z_iopoll_stats_t stats;
  z_opaque_t data;

#if (Z_IOPOLL_ENGINES > 1)
  z_thread_t thread;
#endif /* Z_IOPOLL_ENGINES > 1 */
};

struct z_iopoll {
  const z_vtable_iopoll_t *vtable;
  const int *is_looping;
  int timeout;
  z_iopoll_engine_t engines[Z_IOPOLL_ENGINES];

#if (Z_IOPOLL_ENGINES > 1)
  unsigned int balancer;
#endif /* Z_IOPOLL_ENGINES > 1 */
};

#ifdef Z_IOPOLL_HAS_EPOLL
  extern const z_vtable_iopoll_t z_iopoll_epoll;
#endif /* Z_IOPOLL_HAS_EPOLL */

#ifdef Z_IOPOLL_HAS_KQUEUE
  extern const z_vtable_iopoll_t z_iopoll_kqueue;
#endif /* Z_IOPOLL_HAS_KQUEUE */

#define z_iopoll_is_looping(iopoll)                                          \
  ((iopoll)->is_looping != NULL && *((iopoll)->is_looping))

int   z_iopoll_open         (z_iopoll_t *iopoll,
                             const z_vtable_iopoll_t *vtable);
void  z_iopoll_close        (z_iopoll_t *iopoll);
int   z_iopoll_add          (z_iopoll_t *iopoll,
                             z_iopoll_entity_t *entity);
int   z_iopoll_remove       (z_iopoll_t *iopoll,
                             z_iopoll_entity_t *entity);
int   z_iopoll_poll         (z_iopoll_t *iopoll,
                             const int *is_looping,
                             int timeout);

void  z_iopoll_process      (z_iopoll_t *iopoll,
                             z_iopoll_engine_t *engine,
                             z_iopoll_entity_t *entity,
                             uint32_t events);

void  z_iopoll_entity_init  (z_iopoll_entity_t *entity,
                             const z_vtable_iopoll_entity_t *vtable,
                             int fd);
void z_iopoll_set_writable  (z_iopoll_t *iopoll,
                             z_iopoll_entity_t *entity,
                             int writable);


void z_iopoll_stats_dump            (z_iopoll_engine_t *engine);
void z_iopoll_stats_add_events      (z_iopoll_engine_t *engine,
                                     int nevents,
                                     uint64_t wait_time);
void z_iopoll_stats_add_read_event  (z_iopoll_engine_t *engine, uint64_t time);
void z_iopoll_stats_add_write_event (z_iopoll_engine_t *engine, uint64_t time);

__Z_END_DECLS__

#endif /* _Z_IOPOLL_H_ */
