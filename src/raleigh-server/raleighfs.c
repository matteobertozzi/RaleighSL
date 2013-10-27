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

#include <raleighfs/raleighfs.h>

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
 *  RaleighFS RPC Protocol - Generic
 */
static void __set_status_from_errno (struct status *status, raleighfs_errno_t errno) {
  status->code = errno;
  status_set_code(status);
  if (errno) {
    z_byte_slice_t message;
    raleighfs_errno_byte_slice(errno, &message);
    status->message = z_bytes_from_byteslice(&message);
    status_set_message(status);
    Z_LOG_WARN("Request Failed: %s", raleighfs_errno_string(errno));
  }
  /* TODO: Keep track */
}

static void __operation_completed (raleighfs_t *fs,
                                   uint64_t oid, raleighfs_errno_t errno,
                                   void *ipc_client, void *error_data,
                                   void *req, void *resp)
{
  struct raleighfs_client *client = RALEIGHFS_CLIENT(ipc_client);
  __set_status_from_errno((struct status *)error_data, errno);
  raleighfs_rpc_server_push_response(ipc_client, &(client->msgbuf), req, resp);
}

#define __DECLARE_EXEC(optype, name)                                           \
  static int __rpc_ ## name (z_ipc_client_t *ipc_client,                       \
                             struct name ## _request *req,                     \
                             struct name ## _response *resp)                   \
  {                                                                            \
    struct server_context *ctx = SERVER_CONTEXT(z_global_context_user_data()); \
    name ## _response_set_status(resp);                                        \
    return(raleighfs_exec_ ## optype(&(ctx->fs), req->txn_id, req->oid,        \
                                     __ ## name, __operation_completed,        \
                                     req, resp, ipc_client, &(resp->status))); \
  }

#define __DECLARE_SEMANTIC_EXEC(optype, name)                                  \
  static int __rpc_ ## name (z_ipc_client_t *ipc_client,                       \
                             struct name ## _request *req,                     \
                             struct name ## _response *resp)                   \
  {                                                                            \
    struct server_context *ctx = SERVER_CONTEXT(z_global_context_user_data()); \
    name ## _response_set_status(resp);                                        \
    return(raleighfs_exec_ ## optype(&(ctx->fs),                               \
                                     __ ## name, __operation_completed,        \
                                    req, resp, ipc_client, &(resp->status)));  \
  }

#define __DECLARE_EXEC_READ(name)         __DECLARE_EXEC(read, name)
#define __DECLARE_EXEC_WRITE(name)        __DECLARE_EXEC(write, name)

/* ============================================================================
 *  RaleighFS RPC Protocol - Semantic
 */
static raleighfs_errno_t __semantic_open (raleighfs_t *fs,
                                          void *ipc_client, void *vreq, void *vresp)
{
  const struct semantic_open_request *req = (const struct semantic_open_request *)vreq;
  struct semantic_open_response *resp = (struct semantic_open_response *)vresp;
  raleighfs_errno_t errno;

  if ((errno = raleighfs_semantic_open(fs, req->name, &(resp->oid)))) {
    return(errno);
  }

  semantic_open_response_set_oid(resp);
  return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __semantic_create (raleighfs_t *fs, const raleighfs_object_plug_t *plug,
                                            void *ipc_client, void *vreq, void *vresp)
{
  const struct semantic_create_request *req = (const struct semantic_create_request *)vreq;
  struct semantic_create_response *resp = (struct semantic_create_response *)vresp;
  raleighfs_errno_t errno;

  if ((errno = raleighfs_semantic_create(fs, plug, req->name, &(resp->oid)))) {
    Z_LOG_TRACE("FAILED SEMANTIC CREATE: %s", raleighfs_errno_string(errno));
    return(errno);
  }

  semantic_create_response_set_oid(resp);
  return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __semantic_delete (raleighfs_t *fs,
                                            void *ipc_client, void *vreq, void *vresp)
{
  const struct semantic_delete_request *req = (const struct semantic_delete_request *)vreq;
  raleighfs_errno_t errno;

  if ((errno = raleighfs_semantic_unlink(fs, req->name))) {
    return(errno);
  }

  return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __semantic_rename (raleighfs_t *fs,
                                            void *ipc_client, void *vreq, void *vresp)
{
  const struct semantic_rename_request *req = (const struct semantic_rename_request *)vreq;
  raleighfs_errno_t errno;

  if ((errno = raleighfs_semantic_rename(fs, req->old_name, req->new_name))) {
    return(errno);
  }

  return(RALEIGHFS_ERRNO_NONE);
}

static int __rpc_semantic_create (z_ipc_client_t *ipc_client,
                                  struct semantic_create_request *req,
                                  struct semantic_create_response *resp)
{
  struct server_context *ctx = SERVER_CONTEXT(z_global_context_user_data());
  const raleighfs_object_plug_t *plug;

  semantic_create_response_set_status(resp);

  if ((plug = raleighfs_object_plug_lookup(&(ctx->fs), &(req->type->slice))) == NULL) {
    struct raleighfs_client *client = RALEIGHFS_CLIENT(ipc_client);
    __set_status_from_errno(&(resp->status), RALEIGHFS_ERRNO_PLUGIN_NOT_LOADED);
    return(raleighfs_rpc_server_push_response(ipc_client, &(client->msgbuf), req, resp));
  }

  return(raleighfs_exec_create(&(ctx->fs), plug,
                               __semantic_create, __operation_completed,
                               req, resp, ipc_client, &(resp->status)));
}

__DECLARE_SEMANTIC_EXEC(lookup, semantic_open)
__DECLARE_SEMANTIC_EXEC(unlink, semantic_delete)
__DECLARE_SEMANTIC_EXEC(rename, semantic_rename)

/* ============================================================================
 *  RaleighFS RPC Protocol - Transaction
 */
static int __rpc_transaction_create (z_ipc_client_t *ipc_client,
                                     struct transaction_create_request *req,
                                     struct transaction_create_response *resp)
{
  struct server_context *ctx = SERVER_CONTEXT(z_global_context_user_data());
  struct raleighfs_client *client = RALEIGHFS_CLIENT(ipc_client);
  raleighfs_errno_t errno;

  transaction_create_response_set_status(resp);
  if ((errno = raleighfs_transaction_create(&(ctx->fs), &(resp->txn_id)))) {
    __set_status_from_errno(&(resp->status), errno);
    return(raleighfs_rpc_server_push_response(ipc_client, &(client->msgbuf), req, resp));
  }

  transaction_create_response_set_txn_id(resp);
  return(raleighfs_rpc_server_push_response(ipc_client, &(client->msgbuf), req, resp));
}

static int __rpc_transaction_commit (z_ipc_client_t *ipc_client,
                                     struct transaction_commit_request *req,
                                     struct transaction_commit_response *resp)
{
  struct server_context *ctx = SERVER_CONTEXT(z_global_context_user_data());
  transaction_commit_response_set_status(resp);
  return(raleighfs_exec_txn_commit(&(ctx->fs), req->txn_id,
                                  __operation_completed,
                                  req, resp, ipc_client, &(resp->status)));
}

static int __rpc_transaction_rollback (z_ipc_client_t *ipc_client,
                                       struct transaction_rollback_request *req,
                                       struct transaction_rollback_response *resp)
{
  struct server_context *ctx = SERVER_CONTEXT(z_global_context_user_data());
  transaction_rollback_response_set_status(resp);
  return(raleighfs_exec_txn_rollback(&(ctx->fs), req->txn_id,
                                     __operation_completed,
                                     req, resp, ipc_client, &(resp->status)));
}

/* ============================================================================
 *  RaleighFS RPC Protocol - Counter
 */
static raleighfs_errno_t __counter_get (raleighfs_t *fs,
                                        raleighfs_transaction_t *transaction,
                                        raleighfs_object_t *object,
                                        void *ipc_client, void *vreq, void *vresp)
{
  struct counter_get_response *resp = (struct counter_get_response *)vresp;
  raleighfs_errno_t errno;

  if (Z_UNLIKELY(object->plug != &raleighfs_object_counter))
    return(RALEIGHFS_ERRNO_OBJECT_WRONG_TYPE);

  if ((errno = raleighfs_counter_get(fs, transaction, object, &(resp->value)))) {
    return(errno);
  }

  counter_get_response_set_value(resp);
  return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __counter_set (raleighfs_t *fs,
                                        raleighfs_transaction_t *transaction,
                                        raleighfs_object_t *object,
                                        void *ipc_client, void *vreq, void *vresp)
{
  const struct counter_set_request *req = (struct counter_set_request *)vreq;
  raleighfs_errno_t errno;

  if (Z_UNLIKELY(object->plug != &raleighfs_object_counter))
    return(RALEIGHFS_ERRNO_OBJECT_WRONG_TYPE);

  if ((errno = raleighfs_counter_set(fs, transaction, object, req->value))) {
    return(errno);
  }

  return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __counter_cas (raleighfs_t *fs,
                                        raleighfs_transaction_t *transaction,
                                        raleighfs_object_t *object,
                                        void *ipc_client, void *vreq, void *vresp)
{
  const struct counter_cas_request *req = (struct counter_cas_request *)vreq;
  struct counter_cas_response *resp = (struct counter_cas_response *)vresp;
  raleighfs_errno_t errno;

  if (Z_UNLIKELY(object->plug != &raleighfs_object_counter))
    return(RALEIGHFS_ERRNO_OBJECT_WRONG_TYPE);

  if ((errno = raleighfs_counter_cas(fs, transaction, object,
                                     req->old_value, req->new_value,
                                     &(resp->value))))
  {
    return(errno);
  }

  counter_cas_response_set_value(resp);
  return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __counter_add (raleighfs_t *fs,
                                        raleighfs_transaction_t *transaction,
                                        raleighfs_object_t *object,
                                        void *ipc_client, void *vreq, void *vresp)
{
  const struct counter_add_request *req = (struct counter_add_request *)vreq;
  struct counter_add_response *resp = (struct counter_add_response *)vresp;
  raleighfs_errno_t errno;

  if (Z_UNLIKELY(object->plug != &raleighfs_object_counter))
    return(RALEIGHFS_ERRNO_OBJECT_WRONG_TYPE);

  if ((errno = raleighfs_counter_add(fs, transaction, object, req->value, &(resp->value)))) {
    return(errno);
  }

  counter_add_response_set_value(resp);
  return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __counter_mul (raleighfs_t *fs,
                                        raleighfs_transaction_t *transaction,
                                        raleighfs_object_t *object,
                                        void *ipc_client, void *vreq, void *vresp)
{
  const struct counter_mul_request *req = (struct counter_mul_request *)vreq;
  struct counter_mul_response *resp = (struct counter_mul_response *)vresp;
  raleighfs_errno_t errno;

  if (Z_UNLIKELY(object->plug != &raleighfs_object_counter))
    return(RALEIGHFS_ERRNO_OBJECT_WRONG_TYPE);

  if ((errno = raleighfs_counter_mul(fs, transaction, object, req->value, &(resp->value)))) {
    return(errno);
  }

  counter_mul_response_set_value(resp);
  return(RALEIGHFS_ERRNO_NONE);
}

__DECLARE_EXEC_READ(counter_get)
__DECLARE_EXEC_WRITE(counter_set)
__DECLARE_EXEC_WRITE(counter_cas)
__DECLARE_EXEC_WRITE(counter_add)
__DECLARE_EXEC_WRITE(counter_mul)

/* ============================================================================
 *  RaleighFS RPC Protocol - Sorted-Set
 */
static raleighfs_errno_t __sset_insert (raleighfs_t *fs,
                                        raleighfs_transaction_t *transaction,
                                        raleighfs_object_t *object,
                                        void *ipc_client, void *vreq, void *vresp)
{
  const struct sset_insert_request *req = (const struct sset_insert_request *)vreq;
  raleighfs_errno_t errno;

  if (Z_UNLIKELY(object->plug != &raleighfs_object_sset))
    return(RALEIGHFS_ERRNO_OBJECT_WRONG_TYPE);

  if ((errno = raleighfs_sset_insert(fs, transaction, object,
                                     req->allow_update, req->key, req->value)))
  {
    return(errno);
  }

  return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __sset_update (raleighfs_t *fs,
                                        raleighfs_transaction_t *transaction,
                                        raleighfs_object_t *object,
                                        void *ipc_client, void *vreq, void *vresp)
{
  const struct sset_update_request *req = (const struct sset_update_request *)vreq;
  struct sset_update_response *resp = (struct sset_update_response *)vresp;
  raleighfs_errno_t errno;

  if (Z_UNLIKELY(object->plug != &raleighfs_object_sset))
    return(RALEIGHFS_ERRNO_OBJECT_WRONG_TYPE);

  if ((errno = raleighfs_sset_update(fs, transaction, object, req->key, req->value, &(resp->old_value)))) {
    return(errno);
  }

  if (resp->old_value != NULL) sset_update_response_set_old_value(resp);
  return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __sset_pop (raleighfs_t *fs,
                                     raleighfs_transaction_t *transaction,
                                     raleighfs_object_t *object,
                                     void *ipc_client, void *vreq, void *vresp)
{
  const struct sset_pop_request *req = (const struct sset_pop_request *)vreq;
  struct sset_pop_response *resp = (struct sset_pop_response *)vresp;
  raleighfs_errno_t errno;

  if (Z_UNLIKELY(object->plug != &raleighfs_object_sset))
    return(RALEIGHFS_ERRNO_OBJECT_WRONG_TYPE);

  if ((errno = raleighfs_sset_remove(fs, transaction, object, req->key, &(resp->value)))) {
    return(errno);
  }

  sset_pop_response_set_value(resp);
  return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __sset_get (raleighfs_t *fs,
                                     raleighfs_transaction_t *transaction,
                                     raleighfs_object_t *object,
                                     void *ipc_client, void *vreq, void *vresp)
{
  const struct sset_get_request *req = (const struct sset_get_request *)vreq;
  struct sset_get_response *resp = (struct sset_get_response *)vresp;
  raleighfs_errno_t errno;

  if (Z_UNLIKELY(object->plug != &raleighfs_object_sset))
    return(RALEIGHFS_ERRNO_OBJECT_WRONG_TYPE);

  if ((errno = raleighfs_sset_get(fs, transaction, object, req->key, &(resp->value)))) {
    return(errno);
  }

  sset_get_response_set_value(resp);
  return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __sset_scan (raleighfs_t *fs,
                                      raleighfs_transaction_t *transaction,
                                      raleighfs_object_t *object,
                                      void *ipc_client, void *vreq, void *vresp)
{
  const struct sset_scan_request *req = (const struct sset_scan_request *)vreq;
  struct sset_scan_response *resp = (struct sset_scan_response *)vresp;
  raleighfs_errno_t errno;

  if (Z_UNLIKELY(object->plug != &raleighfs_object_sset))
    return(RALEIGHFS_ERRNO_OBJECT_WRONG_TYPE);

  if ((errno = raleighfs_sset_scan(fs, transaction, object, req->key, req->include_key,
                                   req->count, &(resp->keys), &(resp->values))))
  {
    return(errno);
  }

  sset_scan_response_set_keys(resp);
  sset_scan_response_set_values(resp);
  return(RALEIGHFS_ERRNO_NONE);
}

__DECLARE_EXEC_READ(sset_get)
__DECLARE_EXEC_READ(sset_scan)
__DECLARE_EXEC_WRITE(sset_insert)
__DECLARE_EXEC_WRITE(sset_update)
__DECLARE_EXEC_WRITE(sset_pop)

/* ============================================================================
 *  RaleighFS RPC Protocol - Deque
 */
static raleighfs_errno_t __deque_push (raleighfs_t *fs,
                                       raleighfs_transaction_t *transaction,
                                       raleighfs_object_t *object,
                                       void *ipc_client, void *vreq, void *vresp)
{
  const struct deque_push_request *req = (const struct deque_push_request *)vreq;
  raleighfs_errno_t errno;

  if (Z_UNLIKELY(object->plug != &raleighfs_object_deque))
    return(RALEIGHFS_ERRNO_OBJECT_WRONG_TYPE);

  if ((errno = raleighfs_deque_push(fs, transaction, object, req->front, req->data))) {
    return(errno);
  }

  return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __deque_pop (raleighfs_t *fs,
                                      raleighfs_transaction_t *transaction,
                                      raleighfs_object_t *object,
                                      void *ipc_client, void *vreq, void *vresp)
{
  const struct deque_pop_request *req = (const struct deque_pop_request *)vreq;
  struct deque_pop_response *resp = (struct deque_pop_response *)vresp;
  raleighfs_errno_t errno;

  if (Z_UNLIKELY(object->plug != &raleighfs_object_deque))
    return(RALEIGHFS_ERRNO_OBJECT_WRONG_TYPE);

  if ((errno = raleighfs_deque_pop(fs, transaction, object, req->front, &(resp->data)))) {
    return(errno);
  }

  deque_pop_response_set_data(resp);
  return(RALEIGHFS_ERRNO_NONE);
}

__DECLARE_EXEC_WRITE(deque_push)
__DECLARE_EXEC_WRITE(deque_pop)

/* ============================================================================
 *  RaleighFS RPC Protocol - Server
 */
static int __rpc_server_ping (z_ipc_client_t *ipc_client,
                              struct server_ping_request *req,
                              struct server_ping_response *resp)
{
  struct raleighfs_client *client = RALEIGHFS_CLIENT(ipc_client);
  return(raleighfs_rpc_server_push_response(ipc_client, &(client->msgbuf), req, resp));
}

static int __rpc_server_quit (z_ipc_client_t *ipc_client,
                              struct server_quit_request *req,
                              struct server_quit_response *resp)
{
  return(-1);
}

/* ============================================================================
 *  RaleighFS RPC Protocol
 */
static const struct raleighfs_rpc_server __raleighfs_protocol = {
  /* Semantic */
  .semantic_open   = __rpc_semantic_open,
  .semantic_create = __rpc_semantic_create,
  .semantic_delete = __rpc_semantic_delete,
  .semantic_rename = __rpc_semantic_rename,

  .transaction_create   = __rpc_transaction_create,
  .transaction_commit   = __rpc_transaction_commit,
  .transaction_rollback = __rpc_transaction_rollback,

  /* Counter */
  .counter_get  = __rpc_counter_get,
  .counter_set  = __rpc_counter_set,
  .counter_cas  = __rpc_counter_cas,
  .counter_add  = __rpc_counter_add,
  .counter_mul  = __rpc_counter_mul,

  /* Sorted Set */
  .sset_insert  = __rpc_sset_insert,
  .sset_update  = __rpc_sset_update,
  .sset_pop     = __rpc_sset_pop,
  .sset_get     = __rpc_sset_get,
  .sset_scan    = __rpc_sset_scan,

  /* Deque */
  .deque_push   = __rpc_deque_push,
  .deque_pop    = __rpc_deque_pop,

  /* Server */
  .server_ping = __rpc_server_ping,
  .server_info = NULL,
  .server_quit = __rpc_server_quit,
};

static int __client_msg_parse (z_ipc_client_t *ipc_client, const struct iovec iov[2]) {
  return(raleighfs_rpc_server_parse(&__raleighfs_protocol, ipc_client, iov));
}

/* ============================================================================
 *  RaleighFS IPC Protocol
 */
static int __client_connected (z_ipc_client_t *ipc_client) {
  struct raleighfs_client *client = RALEIGHFS_CLIENT(ipc_client);
  z_ipc_msgbuf_open(&(client->msgbuf), 512);
  Z_LOG_DEBUG("RaleighFS client connected");
  return(0);
}

static void __client_disconnected (z_ipc_client_t *ipc_client) {
  struct raleighfs_client *client = RALEIGHFS_CLIENT(ipc_client);
  Z_LOG_DEBUG("RaleighFS client disconnected");
  z_ipc_msgbuf_close(&(client->msgbuf));
}

static int __client_read (z_ipc_client_t *ipc_client) {
  struct raleighfs_client *client = RALEIGHFS_CLIENT(ipc_client);
  return(z_ipc_msgbuf_fetch(&(client->msgbuf), ipc_client, __client_msg_parse));
}

static int __client_write (z_ipc_client_t *ipc_client) {
  struct raleighfs_client *client = RALEIGHFS_CLIENT(ipc_client);
  return(z_ipc_msgbuf_flush(&(client->msgbuf), ipc_client));
}

const z_ipc_protocol_t raleighfs_protocol = {
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
