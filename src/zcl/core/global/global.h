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

#ifndef _Z_GLOBAL_H_
#define _Z_GLOBAL_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/memory.h>
#include <zcl/iopoll.h>
#include <zcl/vtask.h>

Z_TYPEDEF_STRUCT(z_global_conf)
Z_TYPEDEF_STRUCT(z_msg_stats)

struct z_global_conf {
  uint32_t ncores;

  /* Memory Pool */
  uint32_t mmpool_base_size;
  uint32_t mmpool_page_size;
  uint32_t mmpool_block_min;
  uint32_t mmpoll_block_max;

  /* Events */
  uint32_t local_ring_size;
  uint32_t remote_ring_size;
  uint32_t events_ring_size;

  /* User data */
  uint32_t udata_size;
};

struct z_msg_stats {
  z_histogram_t   req_latency;
  uint64_t        req_latency_events[20];

  z_histogram_t   resp_latency;
  uint64_t        resp_latency_events[20];

  z_histogram_t   task_exec;
  uint64_t        task_exec_events[20];
};

int   z_global_context_open         (z_global_conf_t *conf);
int   z_global_context_open_default (uint32_t ncores);
void  z_global_context_close        (void);
int   z_global_context_poll         (int detached);
void  z_global_context_stop         (void);
void  z_global_context_wait         (void);

void *    z_global_udata        (void);
uint64_t  z_global_uptime       (void);

const z_global_conf_t * z_global_conf     (void);
uint32_t                z_global_balance  (void);

z_memory_t *            z_global_memory       (void);
z_memory_t *            z_global_memory_at    (uint32_t core);

z_iopoll_engine_t *     z_global_iopoll       (void);
z_iopoll_engine_t *     z_global_iopoll_at    (uint32_t core);

const z_iopoll_stats_t *z_global_iopoll_stats    (void);
const z_iopoll_stats_t *z_global_iopoll_stats_at (uint32_t core);

int z_channel_notify    (void);
int z_channel_notify_at (uint32_t core);

int z_channel_add_task    (z_vtask_t *vtask);
int z_channel_add_task_to (z_vtask_t *vtask, uint32_t core);

__Z_END_DECLS__

#endif /* _Z_GLOBAL_H_ */
