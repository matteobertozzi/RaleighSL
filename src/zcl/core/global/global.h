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

#include <zcl/allocator.h>
#include <zcl/task-rq.h>
#include <zcl/macros.h>
#include <zcl/memory.h>

/* global-ctx init/close */
int   z_global_context_open      (z_allocator_t *allocator, void *user_data);
void  z_global_context_close     (void);
void  z_global_context_stop      (void);
void  z_global_context_wait      (void);

/* get/set cpu-ctx user-data */
void *z_global_cpu_ctx        (void);
void *z_global_cpu_ctx_id     (int core);
void  z_global_set_cpu_ctx    (void *udata);
void  z_global_set_cpu_ctx_id (int core, void *udata);
z_task_rq_t *z_global_cpu_rq  (int core);

/* getter for global objects */
z_task_rq_t *  z_global_rq         (void);
z_memory_t *   z_global_memory     (void);
z_allocator_t *z_global_allocator  (void);
const uint8_t *z_global_is_running (void);
unsigned int   z_global_cpu_count  (void);
void *         z_global_user_data  (void);
uint64_t       z_global_uptime     (void);

/* global metrics */
const z_histogram_t *    z_global_histo_memory (int core);
const z_task_rq_stats_t *z_global_rq_stats     (int core);

/* new task notification */
void z_global_new_task_signal     (z_task_rq_t *rq);
void z_global_new_tasks_broadcast (void);

__Z_END_DECLS__

#endif /* _Z_GLOBAL_H_ */
