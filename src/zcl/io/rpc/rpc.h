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

#include <zcl/global.h>
#include <zcl/macros.h>
#include <zcl/reader.h>
#include <zcl/ipc.h>

Z_TYPEDEF_STRUCT(z_rpc_map_node)
Z_TYPEDEF_STRUCT(z_rpc_call)
Z_TYPEDEF_STRUCT(z_rpc_map)
Z_TYPEDEF_STRUCT(z_rpc_ctx)

#define Z_RPC_CTX(x)                    Z_CAST(z_rpc_ctx_t, x)
#define Z_RPC_CTX_REQ(type, ctx)        Z_CAST(type, Z_RPC_CTX(ctx)->req)
#define Z_RPC_CTX_RESP(type, ctx)       Z_CAST(type, Z_RPC_CTX(ctx)->resp)
#define Z_RPC_CTX_CONST_REQ(type, ctx)  Z_CONST_CAST(type, Z_RPC_CTX(ctx)->req)
#define Z_RPC_CTX_CONST_RESP(type, ctx) Z_CONST_CAST(type, Z_RPC_CTX(ctx)->resp)

typedef void (*z_rpc_callback_t)     (z_rpc_ctx_t *ctx, void *ucallback, void *udata);

struct z_rpc_ctx {
  z_iopoll_entity_t *client;
  void *req;
  void *resp;
  uint64_t req_id;
  uint64_t req_time;
  uint64_t msg_type;
  uint8_t blob[1];
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
  z_rwlock_t lock;
  z_rpc_map_node_t *buckets;
  unsigned int size;
  unsigned int mask;
  unsigned int used;
};

struct z_rpc_callbacks {
};

#define z_rpc_ctx_alloc(req_type, resp_type)                                  \
  z_memory_alloc(z_global_memory(), z_rpc_ctx_t,                              \
                 sizeof(z_rpc_ctx_t) - 1 +                                    \
                 sizeof(req_type) + sizeof(resp_type))

#define z_rpc_ctx_init(self, req_type)                                        \
  do {                                                                        \
    (self)->req = (self)->blob;                                               \
    (self)->resp = (self)->blob + sizeof(req_type);                           \
  } while (0)

#define z_rpc_ctx_free(self)                                                  \
  z_memory_free(z_global_memory(), self)


z_rpc_call_t *z_rpc_call_alloc (z_rpc_ctx_t *ctx);
void          z_rpc_call_free  (z_rpc_call_t *self);

int           z_rpc_map_open   (z_rpc_map_t *self);
void          z_rpc_map_close  (z_rpc_map_t *self);
z_rpc_call_t *z_rpc_map_remove (z_rpc_map_t *self, uint64_t req_id);
z_rpc_call_t *z_rpc_map_get    (z_rpc_map_t *self, uint64_t req_id);
z_rpc_call_t *z_rpc_map_add    (z_rpc_map_t *self,
                                z_rpc_ctx_t *ctx,
                                z_rpc_callback_t callback,
                                void *ucallback,
                                void *udata);

#define z_rpc_parse_head(self, msg_type, req_id, is_req)                      \
  z_v_reader_rpc_parse_head(Z_READER_VTABLE(self), self, msg_type, req_id, is_req)

int   z_v_reader_rpc_parse_head (const z_vtable_reader_t *vtable, void *self,
                                 uint64_t *msg_type,
                                 uint64_t *req_id,
                                 int *is_request);
int   z_rpc_write_head          (unsigned char *buffer,
                                 uint64_t msg_type,
                                 uint64_t req_id,
                                 int is_request);

__Z_END_DECLS__

#endif /* _Z_RPC_H_ */
