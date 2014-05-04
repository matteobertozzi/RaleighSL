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

#ifndef _Z_RPC_H_
#define _Z_RPC_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/task-rq.h>
#include <zcl/task.h>
#include <zcl/ipc.h>

Z_TYPEDEF_STRUCT(z_rpc_map_node)
Z_TYPEDEF_STRUCT(z_rpc_task)
Z_TYPEDEF_STRUCT(z_rpc_call)
Z_TYPEDEF_STRUCT(z_rpc_map)
Z_TYPEDEF_STRUCT(z_rpc_ctx)

#define Z_RPC_CTX(x)                    Z_CAST(z_rpc_ctx_t, x)

#define Z_RPC_CTX_DEFINE_REQ(name, type, ctx)                                 \
  const type ## _request_t *name = z_rpc_ctx_request(ctx, type)

#define Z_RPC_CTX_DEFINE_RESP(name, type, ctx)                                \
  type ## _response_t *name = z_rpc_ctx_response(ctx, type)

typedef void (*z_rpc_callback_t) (z_rpc_ctx_t *ctx, void *ucallback, void *udata);

struct z_rpc_ctx {
  z_ipc_msg_client_t *client;
  z_task_rq_group_t group;
  uint64_t req_id;
  uint64_t req_time;
  uint64_t msg_type;
  uint8_t  blob[1];
};

struct z_rpc_task {
  z_task_t vtask;
  z_rpc_ctx_t *ctx;
  void *data;
};

struct z_rpc_call {
  z_rpc_call_t *hash;
  unsigned int refs;
  z_rpc_ctx_t *ctx;
  z_rpc_callback_t callback;
  void *ucallback;
  void *udata;
};

struct z_rpc_map {
};

#define z_rpc_ctx_alloc(req_type, resp_type)                                  \
  __z_rpc_ctx_alloc(sizeof(z_rpc_ctx_t) - 1 +                                 \
                    sizeof(req_type) + sizeof(resp_type))

#define z_rpc_ctx_request(self, type)                                         \
  Z_CAST(type ## _request_t, Z_RPC_CTX(self)->blob)

#define z_rpc_ctx_response(self, type)                                        \
  Z_CAST(type ## _response_t, Z_RPC_CTX(self)->blob + sizeof(type ## _request_t))

#define z_rpc_task_free(self)                                                 \
  z_memory_free(z_global_memory(), self)

z_rpc_ctx_t *__z_rpc_ctx_alloc (size_t size);

void z_rpc_ctx_free (z_rpc_ctx_t *self);

int z_rpc_ctx_add_async (z_rpc_ctx_t *self, z_task_func_t func, void *data);

int           z_rpc_map_open   (z_rpc_map_t *self);
void          z_rpc_map_close  (z_rpc_map_t *self);
z_rpc_call_t *z_rpc_map_remove (z_rpc_map_t *self, uint64_t req_id);
z_rpc_call_t *z_rpc_map_get    (z_rpc_map_t *self, uint64_t req_id);
z_rpc_call_t *z_rpc_map_add    (z_rpc_map_t *self,
                                z_rpc_ctx_t *ctx,
                                z_rpc_callback_t callback,
                                void *ucallback,
                                void *udata);

__Z_END_DECLS__

#endif /* !_Z_RPC_H_ */
