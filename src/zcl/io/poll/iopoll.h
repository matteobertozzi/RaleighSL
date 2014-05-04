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
#include <zcl/ticket.h>
#include <zcl/macros.h>
#include <zcl/thread.h>
#include <zcl/opaque.h>
#include <zcl/system.h>

Z_TYPEDEF_STRUCT(z_vtable_iopoll_entity)
Z_TYPEDEF_STRUCT(z_iopoll_entity)
Z_TYPEDEF_STRUCT(z_iopoll_stats)

#define Z_IOPOLL_ENTITY_FD(x)       (Z_IOPOLL_ENTITY(x)->fd)
#define Z_IOPOLL_ENTITY(x)          Z_CAST(z_iopoll_entity_t, x)

#define __Z_IOPOLL_ENTITY__         z_iopoll_entity_t __io_entity__;

struct z_vtable_iopoll_entity {
  int     (*read)     (z_iopoll_entity_t *entity);
  int     (*write)    (z_iopoll_entity_t *entity);
  void    (*close)    (z_iopoll_entity_t *entity);
};

struct z_iopoll_entity {
  z_ticket_t lock;
  uint8_t    type;
  uint8_t    iflags8;
  uint8_t    ioengine;
  uint8_t    uflags8;
  uint32_t   last_write_ts;
  int        fd;
};

struct z_iopoll_stats {
  z_histogram_t iowait;
  uint64_t      iowait_events[24];

  z_histogram_t ioread;
  uint64_t      ioread_events[24];

  z_histogram_t iowrite;
  uint64_t      iowrite_events[24];

  z_histogram_t req_latency;
  uint64_t      req_latency_events[24];

  z_histogram_t resp_latency;
  uint64_t      resp_latency_events[24];
};

int  z_iopoll_open  (unsigned int ncores);
void z_iopoll_close (void);
void z_iopoll_stop  (void);
void z_iopoll_wait  (void);

int  z_iopoll_add     (z_iopoll_entity_t *entity);
int  z_iopoll_remove  (z_iopoll_entity_t *entity);
int  z_iopoll_poll    (int detached, const uint8_t *is_looping);

unsigned int            z_iopoll_cores (void);
const z_iopoll_stats_t *z_iopoll_stats (int core);

void z_iopoll_entity_open   (z_iopoll_entity_t *entity,
                             const z_vtable_iopoll_entity_t *vtable,
                             int fd);
void z_iopoll_entity_close  (z_iopoll_entity_t *entity, int defer);
void z_iopoll_set_writable  (z_iopoll_entity_t *entity, int writable);
void z_iopoll_add_latencies (z_iopoll_entity_t *entity,
                             uint64_t req_latency,
                             uint64_t resp_latency);

__Z_END_DECLS__

#endif /* !_Z_IOPOLL_H_ */
