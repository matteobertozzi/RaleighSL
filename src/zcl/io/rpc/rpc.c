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

#include <zcl/global.h>
#include <zcl/memory.h>
#include <zcl/task.h>
#include <zcl/rpc.h>

/* ============================================================================
 *  PRIVATE RPC ctx methods
 */
z_rpc_ctx_t *__z_rpc_ctx_alloc (size_t size) {
  z_rpc_ctx_t *ctx;

  ctx = z_memory_alloc(z_global_memory(), z_rpc_ctx_t, size);
  if (Z_MALLOC_IS_NULL(ctx))
    return(NULL);

  z_task_group_open(&(ctx->group), 0);
  return(ctx);
}

void z_rpc_ctx_free (z_rpc_ctx_t *self) {
  z_task_group_close(&(self->group));

  z_memory_set_dirty_debug(&(self->group), sizeof(z_task_rq_group_t));
  z_memory_free(z_global_memory(), self);
}

int z_rpc_ctx_add_async (z_rpc_ctx_t *self, z_task_func_t func, void *data) {
  z_rpc_task_t *task;

  task = z_memory_struct_alloc(z_global_memory(), z_rpc_task_t);
  if (Z_MALLOC_IS_NULL(task))
    return(-1);

  z_task_reset(&(task->vtask), func);
  task->vtask.vtask.flags.autoclean = 1;
  task->ctx = self;
  task->data = data;

  z_task_group_add_barrier(&(self->group), &(task->vtask.vtask));
  return(0);
}