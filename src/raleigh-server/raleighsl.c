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

#include <raleighsl/raleighsl.h>

#include <zcl/coding.h>
#include <zcl/global.h>
#include <zcl/string.h>
#include <zcl/iovec.h>
#include <zcl/debug.h>
#include <zcl/time.h>
#include <zcl/ipc.h>

#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>

#include "rpc/generated/rpc_server.h"
#include "server.h"

/* ============================================================================
 *  RaleighSL RPC Protocol - Generic
 */
static void __set_status_from_errno (struct status *status, raleighsl_errno_t errno) {
  status->code = errno;
  status_set_code(status);
  if (errno) {
    z_byte_slice_t message;
    raleighsl_errno_byte_slice(errno, &message);
    z_bytes_ref_set(&(status->message), &message, NULL, NULL);
    status_set_message(status);
    Z_LOG_TRACE("Request Failed: %s", raleighsl_errno_string(errno));
  }
  /* TODO: Keep track for poll */
}

static void __operation_completed (raleighsl_t *fs,
                                   uint64_t oid, raleighsl_errno_t errno,
                                   void *ctx, void *error_data)
{
  struct raleighsl_client *client = RALEIGHSL_CLIENT(Z_RPC_CTX(ctx)->client);
  __set_status_from_errno((struct status *)error_data, errno);
  raleighsl_rpc_server_push_response(Z_RPC_CTX(ctx), &(client->msgbuf));
}

#define __VERIFY_OBJ_PLUG_TYPE(obj, type)                                      \
  if (Z_UNLIKELY((obj)->plug != &raleighsl_object_ ## type))                   \
    return(RALEIGHSL_ERRNO_OBJECT_WRONG_TYPE);

#define __DECLARE_EXEC(optype, name)                                           \
  static int __rpc_ ## name (z_rpc_ctx_t *ctx,                                 \
                             struct name ## _request *req,                     \
                             struct name ## _response *resp)                   \
  {                                                                            \
    struct server_context *srv = SERVER_CONTEXT(z_global_context_user_data()); \
    name ## _response_set_status(resp);                                        \
    return(raleighsl_exec_ ## optype(&(srv->fs), req->txn_id, req->oid,        \
                                     __ ## name, __operation_completed,        \
                                     ctx, &(resp->status)));                   \
  }

#define __DECLARE_SEMANTIC_EXEC(optype, name)                                  \
  static int __rpc_ ## name (z_rpc_ctx_t *ctx,                                 \
                             struct name ## _request *req,                     \
                             struct name ## _response *resp)                   \
  {                                                                            \
    struct server_context *srv = SERVER_CONTEXT(z_global_context_user_data()); \
    name ## _response_set_status(resp);                                        \
    return(raleighsl_exec_ ## optype(&(srv->fs),                               \
                                     __ ## name, __operation_completed,        \
                                    ctx, &(resp->status)));                    \
  }

#define __DECLARE_EXEC_READ(name)         __DECLARE_EXEC(read, name)
#define __DECLARE_EXEC_WRITE(name)        __DECLARE_EXEC(write, name)

/* ============================================================================
 *  RaleighSL RPC Protocol - Semantic
 */
static raleighsl_errno_t __semantic_open (raleighsl_t *fs, void *ctx) {
  const struct semantic_open_request *req = Z_RPC_CTX_CONST_REQ(struct semantic_open_request, ctx);
  struct semantic_open_response *resp = Z_RPC_CTX_RESP(struct semantic_open_response, ctx);
  raleighsl_errno_t errno;

  if ((errno = raleighsl_semantic_open(fs, &(req->name), &(resp->oid)))) {
    return(errno);
  }

  semantic_open_response_set_oid(resp);
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __semantic_create (raleighsl_t *fs,
                                            const raleighsl_object_plug_t *plug,
                                            void *ctx)
{
  const struct semantic_create_request *req = Z_RPC_CTX_CONST_REQ(struct semantic_create_request, ctx);
  struct semantic_create_response *resp = Z_RPC_CTX_RESP(struct semantic_create_response, ctx);
  raleighsl_errno_t errno;

  if ((errno = raleighsl_semantic_create(fs, plug, &(req->name), &(resp->oid)))) {
    Z_LOG_TRACE("FAILED SEMANTIC CREATE: %s", raleighsl_errno_string(errno));
    return(errno);
  }

  semantic_create_response_set_oid(resp);
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __semantic_delete (raleighsl_t *fs, void *ctx) {
  const struct semantic_delete_request *req = Z_RPC_CTX_CONST_REQ(struct semantic_delete_request, ctx);
  raleighsl_errno_t errno;

  if ((errno = raleighsl_semantic_unlink(fs, &(req->name)))) {
    return(errno);
  }

  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __semantic_rename (raleighsl_t *fs, void *ctx) {
  const struct semantic_rename_request *req = Z_RPC_CTX_CONST_REQ(struct semantic_rename_request, ctx);
  raleighsl_errno_t errno;

  if ((errno = raleighsl_semantic_rename(fs, &(req->old_name), &(req->new_name)))) {
    return(errno);
  }

  return(RALEIGHSL_ERRNO_NONE);
}

static int __rpc_semantic_create (z_rpc_ctx_t *ctx,
                                  struct semantic_create_request *req,
                                  struct semantic_create_response *resp)
{
  struct server_context *srv = SERVER_CONTEXT(z_global_context_user_data());
  const raleighsl_object_plug_t *plug;

  semantic_create_response_set_status(resp);

  if ((plug = raleighsl_object_plug_lookup(&(srv->fs), &(req->type.slice))) == NULL) {
    struct raleighsl_client *client = RALEIGHSL_CLIENT(ctx->client);
    __set_status_from_errno(&(resp->status), RALEIGHSL_ERRNO_PLUGIN_NOT_LOADED);
    return(raleighsl_rpc_server_push_response(ctx, &(client->msgbuf)));
  }

  return(raleighsl_exec_create(&(srv->fs), plug,
                               __semantic_create, __operation_completed,
                               ctx, &(resp->status)));
}

__DECLARE_SEMANTIC_EXEC(lookup, semantic_open)
__DECLARE_SEMANTIC_EXEC(unlink, semantic_delete)
__DECLARE_SEMANTIC_EXEC(rename, semantic_rename)

/* ============================================================================
 *  RaleighSL RPC Protocol - Transaction
 */
static int __rpc_transaction_create (z_rpc_ctx_t *ctx,
                                     struct transaction_create_request *req,
                                     struct transaction_create_response *resp)
{
  struct server_context *srv = SERVER_CONTEXT(z_global_context_user_data());
  struct raleighsl_client *client = RALEIGHSL_CLIENT(ctx->client);
  raleighsl_errno_t errno;

  transaction_create_response_set_status(resp);
  if ((errno = raleighsl_transaction_create(&(srv->fs), &(resp->txn_id)))) {
    __set_status_from_errno(&(resp->status), errno);
    return(raleighsl_rpc_server_push_response(ctx, &(client->msgbuf)));
  }

  transaction_create_response_set_txn_id(resp);
  return(raleighsl_rpc_server_push_response(ctx, &(client->msgbuf)));
}

static int __rpc_transaction_commit (z_rpc_ctx_t *ctx,
                                     struct transaction_commit_request *req,
                                     struct transaction_commit_response *resp)
{
  struct server_context *srv = SERVER_CONTEXT(z_global_context_user_data());
  transaction_commit_response_set_status(resp);
  return(raleighsl_exec_txn_commit(&(srv->fs), req->txn_id,
                                  __operation_completed,
                                  ctx, &(resp->status)));
}

static int __rpc_transaction_rollback (z_rpc_ctx_t *ctx,
                                       struct transaction_rollback_request *req,
                                       struct transaction_rollback_response *resp)
{
  struct server_context *srv = SERVER_CONTEXT(z_global_context_user_data());
  transaction_rollback_response_set_status(resp);
  return(raleighsl_exec_txn_rollback(&(srv->fs), req->txn_id,
                                     __operation_completed,
                                     ctx, &(resp->status)));
}

/* ============================================================================
 *  RaleighSL RPC Protocol - Counter
 */
static raleighsl_errno_t __number_get (raleighsl_t *fs,
                                       const raleighsl_transaction_t *transaction,
                                       raleighsl_object_t *object,
                                       void *ctx)
{
  struct number_get_response *resp = Z_RPC_CTX_RESP(struct number_get_response, ctx);
  raleighsl_errno_t errno;

  __VERIFY_OBJ_PLUG_TYPE(object, number);
  if ((errno = raleighsl_number_get(fs, transaction, object, &(resp->value)))) {
    return(errno);
  }

  number_get_response_set_value(resp);
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __number_set (raleighsl_t *fs,
                                       raleighsl_transaction_t *transaction,
                                       raleighsl_object_t *object,
                                       void *ctx)
{
  const struct number_set_request *req = Z_RPC_CTX_CONST_REQ(struct number_set_request, ctx);
  raleighsl_errno_t errno;

  __VERIFY_OBJ_PLUG_TYPE(object, number);
  if ((errno = raleighsl_number_set(fs, transaction, object, req->value))) {
    return(errno);
  }

  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __number_cas (raleighsl_t *fs,
                                       raleighsl_transaction_t *transaction,
                                       raleighsl_object_t *object,
                                       void *ctx)
{
  const struct number_cas_request *req = Z_RPC_CTX_CONST_REQ(struct number_cas_request, ctx);
  struct number_cas_response *resp = Z_RPC_CTX_RESP(struct number_cas_response, ctx);
  raleighsl_errno_t errno;

  __VERIFY_OBJ_PLUG_TYPE(object, number);
  if ((errno = raleighsl_number_cas(fs, transaction, object,
                                     req->old_value, req->new_value,
                                     &(resp->value))))
  {
    return(errno);
  }

  number_cas_response_set_value(resp);
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __number_add (raleighsl_t *fs,
                                       raleighsl_transaction_t *transaction,
                                       raleighsl_object_t *object,
                                       void *ctx)
{
  const struct number_add_request *req = Z_RPC_CTX_CONST_REQ(struct number_add_request, ctx);
  struct number_add_response *resp = Z_RPC_CTX_RESP(struct number_add_response, ctx);
  raleighsl_errno_t errno;

  __VERIFY_OBJ_PLUG_TYPE(object, number);
  if ((errno = raleighsl_number_add(fs, transaction, object, req->value, &(resp->value)))) {
    return(errno);
  }

  number_add_response_set_value(resp);
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __number_mul (raleighsl_t *fs,
                                       raleighsl_transaction_t *transaction,
                                       raleighsl_object_t *object,
                                       void *ctx)
{
  const struct number_mul_request *req = Z_RPC_CTX_CONST_REQ(struct number_mul_request, ctx);
  struct number_mul_response *resp = Z_RPC_CTX_RESP(struct number_mul_response, ctx);
  raleighsl_errno_t errno;

  __VERIFY_OBJ_PLUG_TYPE(object, number);
  if ((errno = raleighsl_number_mul(fs, transaction, object, req->value, &(resp->value)))) {
    return(errno);
  }

  number_mul_response_set_value(resp);
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __number_div (raleighsl_t *fs,
                                       raleighsl_transaction_t *transaction,
                                       raleighsl_object_t *object,
                                       void *ctx)
{
  const struct number_div_request *req = Z_RPC_CTX_CONST_REQ(struct number_div_request, ctx);
  struct number_div_response *resp = Z_RPC_CTX_RESP(struct number_div_response, ctx);
  raleighsl_errno_t errno;

  __VERIFY_OBJ_PLUG_TYPE(object, number);
  if ((errno = raleighsl_number_div(fs, transaction, object, req->value,
                                    &(resp->mod), &(resp->value))))
  {
    return(errno);
  }

  number_div_response_set_mod(resp);
  number_div_response_set_value(resp);
  return(RALEIGHSL_ERRNO_NONE);
}

__DECLARE_EXEC_READ(number_get)
__DECLARE_EXEC_WRITE(number_set)
__DECLARE_EXEC_WRITE(number_cas)
__DECLARE_EXEC_WRITE(number_add)
__DECLARE_EXEC_WRITE(number_mul)
__DECLARE_EXEC_WRITE(number_div)

/* ============================================================================
 *  RaleighSL RPC Protocol - Sorted-Set
 */
static raleighsl_errno_t __sset_insert (raleighsl_t *fs,
                                        raleighsl_transaction_t *transaction,
                                        raleighsl_object_t *object,
                                        void *ctx)
{
  const struct sset_insert_request *req = Z_RPC_CTX_CONST_REQ(struct sset_insert_request, ctx);
  raleighsl_errno_t errno;

  __VERIFY_OBJ_PLUG_TYPE(object, sset);
  if ((errno = raleighsl_sset_insert(fs, transaction, object,
                                     req->allow_update, &(req->key), &(req->value))))
  {
    return(errno);
  }

  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __sset_update (raleighsl_t *fs,
                                        raleighsl_transaction_t *transaction,
                                        raleighsl_object_t *object,
                                        void *ctx)
{
  const struct sset_update_request *req = Z_RPC_CTX_CONST_REQ(struct sset_update_request, ctx);
  struct sset_update_response *resp = Z_RPC_CTX_RESP(struct sset_update_response, ctx);
  raleighsl_errno_t errno;

  __VERIFY_OBJ_PLUG_TYPE(object, sset);
  if ((errno = raleighsl_sset_update(fs, transaction, object,
                                     &(req->key), &(req->value), &(resp->old_value))))
  {
    return(errno);
  }

  if (z_byte_slice_is_not_empty(&(resp->old_value)))
    sset_update_response_set_old_value(resp);
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __sset_pop (raleighsl_t *fs,
                                     raleighsl_transaction_t *transaction,
                                     raleighsl_object_t *object,
                                     void *ctx)
{
  const struct sset_pop_request *req = Z_RPC_CTX_CONST_REQ(struct sset_pop_request, ctx);
  struct sset_pop_response *resp = Z_RPC_CTX_RESP(struct sset_pop_response, ctx);
  raleighsl_errno_t errno;

  __VERIFY_OBJ_PLUG_TYPE(object, sset);
  if ((errno = raleighsl_sset_remove(fs, transaction, object, &(req->key), &(resp->value)))) {
    return(errno);
  }

  sset_pop_response_set_value(resp);
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __sset_get (raleighsl_t *fs,
                                     const raleighsl_transaction_t *transaction,
                                     raleighsl_object_t *object,
                                     void *ctx)
{
  const struct sset_get_request *req = Z_RPC_CTX_CONST_REQ(struct sset_get_request, ctx);
  struct sset_get_response *resp = Z_RPC_CTX_RESP(struct sset_get_response, ctx);
  raleighsl_errno_t errno;

  __VERIFY_OBJ_PLUG_TYPE(object, sset);
  if ((errno = raleighsl_sset_get(fs, transaction, object, &(req->key), &(resp->value)))) {
    return(errno);
  }

  sset_get_response_set_value(resp);
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __sset_scan (raleighsl_t *fs,
                                      const raleighsl_transaction_t *transaction,
                                      raleighsl_object_t *object,
                                      void *ctx)
{
  const struct sset_scan_request *req = Z_RPC_CTX_CONST_REQ(struct sset_scan_request, ctx);
  struct sset_scan_response *resp = Z_RPC_CTX_RESP(struct sset_scan_response, ctx);
  raleighsl_errno_t errno;

  __VERIFY_OBJ_PLUG_TYPE(object, sset);
  if ((errno = raleighsl_sset_scan(fs, transaction, object, &(req->key), req->include_key,
                                   req->count, &(resp->keys), &(resp->values))))
  {
    return(errno);
  }

  sset_scan_response_set_keys(resp);
  sset_scan_response_set_values(resp);
  return(RALEIGHSL_ERRNO_NONE);
}

__DECLARE_EXEC_READ(sset_get)
__DECLARE_EXEC_READ(sset_scan)
__DECLARE_EXEC_WRITE(sset_insert)
__DECLARE_EXEC_WRITE(sset_update)
__DECLARE_EXEC_WRITE(sset_pop)

/* ============================================================================
 *  RaleighSL RPC Protocol - Flow
 */
static raleighsl_errno_t __flow_append (raleighsl_t *fs,
                                        raleighsl_transaction_t *transaction,
                                        raleighsl_object_t *object,
                                        void *ctx)
{
  const struct flow_append_request *req = Z_RPC_CTX_CONST_REQ(struct flow_append_request, ctx);
  struct flow_append_response *resp = Z_RPC_CTX_RESP(struct flow_append_response, ctx);
  raleighsl_errno_t errno;

  __VERIFY_OBJ_PLUG_TYPE(object, flow);
  errno = raleighsl_flow_append(fs, transaction, object, &(req->data), &(resp->size));
  if (Z_UNLIKELY(errno)) {
    return(errno);
  }

  flow_append_response_set_size(resp);
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __flow_inject (raleighsl_t *fs,
                                        raleighsl_transaction_t *transaction,
                                        raleighsl_object_t *object,
                                        void *ctx)
{
  const struct flow_inject_request *req = Z_RPC_CTX_CONST_REQ(struct flow_inject_request, ctx);
  struct flow_inject_response *resp = Z_RPC_CTX_RESP(struct flow_inject_response, ctx);
  raleighsl_errno_t errno;

  __VERIFY_OBJ_PLUG_TYPE(object, flow);
  errno = raleighsl_flow_inject(fs, transaction, object, req->offset, &(req->data), &(resp->size));
  if (Z_UNLIKELY(errno)) {
    return(errno);
  }

  flow_inject_response_set_size(resp);
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __flow_write (raleighsl_t *fs,
                                       raleighsl_transaction_t *transaction,
                                       raleighsl_object_t *object,
                                       void *ctx)
{
  const struct flow_write_request *req = Z_RPC_CTX_CONST_REQ(struct flow_write_request, ctx);
  struct flow_write_response *resp = Z_RPC_CTX_RESP(struct flow_write_response, ctx);
  raleighsl_errno_t errno;

  __VERIFY_OBJ_PLUG_TYPE(object, flow);
  errno = raleighsl_flow_write(fs, transaction, object, req->offset, &(req->data), &(resp->size));
  if (Z_UNLIKELY(errno)) {
    return(errno);
  }

  flow_write_response_set_size(resp);
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __flow_remove (raleighsl_t *fs,
                                        raleighsl_transaction_t *transaction,
                                        raleighsl_object_t *object,
                                        void *ctx)
{
  const struct flow_remove_request *req = Z_RPC_CTX_CONST_REQ(struct flow_remove_request, ctx);
  struct flow_remove_response *resp = Z_RPC_CTX_RESP(struct flow_remove_response, ctx);
  raleighsl_errno_t errno;

  __VERIFY_OBJ_PLUG_TYPE(object, flow);
  errno = raleighsl_flow_remove(fs, transaction, object, req->offset, req->size, &(resp->size));
  if (Z_UNLIKELY(errno)) {
    return(errno);
  }

  flow_remove_response_set_size(resp);
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __flow_truncate (raleighsl_t *fs,
                                          raleighsl_transaction_t *transaction,
                                          raleighsl_object_t *object,
                                          void *ctx)
{
  const struct flow_truncate_request *req = Z_RPC_CTX_CONST_REQ(struct flow_truncate_request, ctx);
  struct flow_truncate_response *resp = Z_RPC_CTX_RESP(struct flow_truncate_response, ctx);
  raleighsl_errno_t errno;

  __VERIFY_OBJ_PLUG_TYPE(object, flow);
  errno = raleighsl_flow_truncate(fs, transaction, object, req->size, &(resp->size));
  if (Z_UNLIKELY(errno)) {
    return(errno);
  }

  flow_truncate_response_set_size(resp);
  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __flow_read (raleighsl_t *fs,
                                      const raleighsl_transaction_t *transaction,
                                      raleighsl_object_t *object,
                                      void *ctx)
{
  const struct flow_read_request *req = Z_RPC_CTX_CONST_REQ(struct flow_read_request, ctx);
  struct flow_read_response *resp = Z_RPC_CTX_RESP(struct flow_read_response, ctx);
  raleighsl_errno_t errno;

  __VERIFY_OBJ_PLUG_TYPE(object, flow);
  errno = raleighsl_flow_read(fs, transaction, object, req->offset, req->size, &(resp->data));
  if (Z_UNLIKELY(errno)) {
    return(errno);
  }

  flow_read_response_set_data(resp);
  return(RALEIGHSL_ERRNO_NONE);
}

__DECLARE_EXEC_WRITE(flow_append)
__DECLARE_EXEC_WRITE(flow_inject)
__DECLARE_EXEC_WRITE(flow_write)
__DECLARE_EXEC_WRITE(flow_remove)
__DECLARE_EXEC_WRITE(flow_truncate)
__DECLARE_EXEC_READ(flow_read)

/* ============================================================================
 *  RaleighSL RPC Protocol - Deque
 */
static raleighsl_errno_t __deque_push (raleighsl_t *fs,
                                       raleighsl_transaction_t *transaction,
                                       raleighsl_object_t *object,
                                       void *ctx)
{
  const struct deque_push_request *req = Z_RPC_CTX_CONST_REQ(struct deque_push_request, ctx);
  raleighsl_errno_t errno;

  __VERIFY_OBJ_PLUG_TYPE(object, deque);
  if ((errno = raleighsl_deque_push(fs, transaction, object,
                                    req->front, &(req->data))))
  {
    return(errno);
  }

  return(RALEIGHSL_ERRNO_NONE);
}

static raleighsl_errno_t __deque_pop (raleighsl_t *fs,
                                      raleighsl_transaction_t *transaction,
                                      raleighsl_object_t *object,
                                      void *ctx)
{
  const struct deque_pop_request *req = Z_RPC_CTX_CONST_REQ(struct deque_pop_request, ctx);
  struct deque_pop_response *resp = Z_RPC_CTX_RESP(struct deque_pop_response, ctx);
  raleighsl_errno_t errno;

  __VERIFY_OBJ_PLUG_TYPE(object, deque);
  if ((errno = raleighsl_deque_pop(fs, transaction, object,
                                   req->front, &(resp->data))))
  {
    return(errno);
  }

  deque_pop_response_set_data(resp);
  return(RALEIGHSL_ERRNO_NONE);
}

__DECLARE_EXEC_WRITE(deque_push)
__DECLARE_EXEC_WRITE(deque_pop)

/* ============================================================================
 *  RaleighSL RPC Protocol - Server
 */
static int __rpc_server_ping (z_rpc_ctx_t *ctx,
                              struct server_ping_request *req,
                              struct server_ping_response *resp)
{
  struct raleighsl_client *client = RALEIGHSL_CLIENT(ctx->client);
  return(raleighsl_rpc_server_push_response(ctx, &(client->msgbuf)));
}

static int __rpc_server_quit (z_rpc_ctx_t *ctx,
                              struct server_quit_request *req,
                              struct server_quit_response *resp)
{
  return(-1);
}

static int __rpc_server_debug (z_rpc_ctx_t *ctx,
                               struct server_debug_request *req,
                               struct server_debug_response *resp)
{
  struct raleighsl_client *client = RALEIGHSL_CLIENT(ctx->client);
  z_debug_set_log_level(req->log_level);
  return(raleighsl_rpc_server_push_response(ctx, &(client->msgbuf)));
}

/* ============================================================================
 *  RaleighSL RPC Protocol
 */
static const struct raleighsl_rpc_server __raleighsl_protocol = {
  /* Semantic */
  .semantic_open   = __rpc_semantic_open,
  .semantic_create = __rpc_semantic_create,
  .semantic_delete = __rpc_semantic_delete,
  .semantic_rename = __rpc_semantic_rename,

  .transaction_create   = __rpc_transaction_create,
  .transaction_commit   = __rpc_transaction_commit,
  .transaction_rollback = __rpc_transaction_rollback,

  /* Counter */
  .number_get   = __rpc_number_get,
  .number_set   = __rpc_number_set,
  .number_cas   = __rpc_number_cas,
  .number_add   = __rpc_number_add,
  .number_mul   = __rpc_number_mul,
  .number_div   = __rpc_number_div,

  /* Sorted Set */
  .sset_insert  = __rpc_sset_insert,
  .sset_update  = __rpc_sset_update,
  .sset_pop     = __rpc_sset_pop,
  .sset_get     = __rpc_sset_get,
  .sset_scan    = __rpc_sset_scan,

  /* Flow */
  .flow_append   = __rpc_flow_append,
  .flow_inject   = __rpc_flow_inject,
  .flow_write    = __rpc_flow_write,
  .flow_remove   = __rpc_flow_remove,
  .flow_truncate = __rpc_flow_truncate,
  .flow_read     = __rpc_flow_read,

  /* Deque */
  .deque_push   = __rpc_deque_push,
  .deque_pop    = __rpc_deque_pop,

  /* Server */
  .server_ping  = __rpc_server_ping,
  .server_info  = NULL,
  .server_quit  = __rpc_server_quit,
  .server_debug = __rpc_server_debug,
};

static int __client_msg_parse (z_iopoll_entity_t *ipc_client, const struct iovec iov[2]) {
  return(raleighsl_rpc_server_parse(&__raleighsl_protocol, Z_IPC_CLIENT(ipc_client), iov));
}

/* ============================================================================
 *  RaleighSL IPC Protocol
 */
static int __client_connected (z_ipc_client_t *ipc_client) {
  struct raleighsl_client *client = RALEIGHSL_CLIENT(ipc_client);
  z_ipc_msgbuf_open(&(client->msgbuf), 512);
  Z_LOG_DEBUG("RaleighSL client connected");
  return(0);
}

static void __client_disconnected (z_ipc_client_t *ipc_client) {
  struct raleighsl_client *client = RALEIGHSL_CLIENT(ipc_client);
  Z_LOG_DEBUG("RaleighSL client disconnected");
  z_ipc_msgbuf_close(&(client->msgbuf));
}

static int __client_read (z_ipc_client_t *ipc_client) {
  struct raleighsl_client *client = RALEIGHSL_CLIENT(ipc_client);
  return(z_ipc_msgbuf_fetch(&(client->msgbuf), Z_IOPOLL_ENTITY(ipc_client), __client_msg_parse));
}

static int __client_write (z_ipc_client_t *ipc_client) {
  struct raleighsl_client *client = RALEIGHSL_CLIENT(ipc_client);
  return(z_ipc_msgbuf_flush(&(client->msgbuf), z_ipc_client_iopoll(client), Z_IOPOLL_ENTITY(ipc_client)));
}

const z_ipc_protocol_t raleighsl_protocol = {
  /* server protocol */
  .bind         = z_ipc_bind_tcp,
  .accept       = z_ipc_accept_tcp,
  .setup        = NULL,

  /* client protocol */
  .connected    = __client_connected,
  .disconnected = __client_disconnected,
  .read         = __client_read,
  .write        = __client_write,
};
