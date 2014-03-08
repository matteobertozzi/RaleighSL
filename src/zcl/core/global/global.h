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
#include <zcl/macros.h>
#include <zcl/memory.h>
#include <zcl/task.h>

Z_TYPEDEF_STRUCT(z_global_context)

int   z_global_context_open      (z_allocator_t *allocator, void *user_data);
void  z_global_context_close     (void);
void  z_global_context_stop      (void);
void *z_global_context_user_data (void);

z_memory_t *  z_global_memory     (void);

void z_global_add_task (z_task_t *task);
void z_global_add_pending_tasks (z_task_t *tasks);
void z_global_add_pending_ntasks (int count, ...);

__Z_END_DECLS__

#endif /* _Z_GLOBAL_H_ */
