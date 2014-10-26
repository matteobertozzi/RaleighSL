/*
 *   Copyright 2007-2013 Matteo Bertozzi
 *
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

#include <zcl/debug.h>
#include <zcl/time.h>
#include <zcl/ipc.h>
#include <zcl/fd.h>

#include "server.h"
#include "rpc/generated/r5l.h"

#if 0
{
  .transaction_create = {
    .alloc  = ...
    .parse  = ...
    .exec   = ...
    .states = &__txn_states,
  }
}
#endif

/* ============================================================================
 *  IPC Protocol
 */
static int __client_connected (z_ipc_client_t *client) {
  struct r5l_client *r5l = (struct r5l_client *)client;
  Z_LOG_INFO("r5l client connected %p", r5l);
  return(0);
}

static int __client_disconnected (z_ipc_client_t *client) {
  struct r5l_client *r5l = (struct r5l_client *)client;
  Z_LOG_INFO("r5l client disconnected %p", r5l);
  return(0);
}

static int __client_msg_alloc (z_ipc_msg_client_t *client,
                               z_ipc_msg_head_t *msg_head)
{
  Z_LOG_DEBUG("msg_id: %"PRIu64" msg_type: %"PRIu32
              " body_len: %"PRIu16" blob_length: %"PRIu32,
              msg_head->msg_id, msg_head->msg_type,
              msg_head->body_length, msg_head->blob_length);
  return(0);
}

static int __client_msg_parse (z_ipc_msg_client_t *client,
                               z_ipc_msg_head_t *msg_head)
{
  Z_LOG_DEBUG("msg_id: %"PRIu64" msg_type: %"PRIu32
              " body_len: %"PRIu16" blob_length: %"PRIu32,
              msg_head->msg_id, msg_head->msg_type,
              msg_head->body_length, msg_head->blob_length);
  return(0);
}

#include <zcl/debug.h>
static int __client_msg_exec (z_ipc_msg_client_t *client,
                              z_ipc_msg_head_t *msg_head)
{
  Z_LOG_DEBUG("msg_id: %"PRIu64" msg_type: %"PRIu32
              " body_len: %"PRIu16" blob_length: %"PRIu32,
              msg_head->msg_id, msg_head->msg_type,
              msg_head->body_length, msg_head->blob_length);

  z_dbuf_writer_t writer;
  z_ipc_msg_t *msg;
  msg = z_ipc_msg_writer_open(NULL, msg_head, &writer, z_global_memory());
  z_ipc_msg_writer_close(msg, &writer, 0, 0);
  z_ipc_msg_client_push(client, msg);

  z_ipc_client_set_data_available(client, 1);
  return(0);
}

const z_ipc_protocol_t r5l_tcp_protocol = {
  /* raw-client protocol */
  .read         = NULL,
  .write        = NULL,

  /* client protocol */
  .connected    = __client_connected,
  .disconnected = __client_disconnected,

  /* server protocol */
  .uevent       = NULL,
  .timeout      = NULL,
  .accept       = z_ipc_accept_tcp,
  .bind         = z_ipc_bind_tcp,
  .unbind       = NULL,
  .setup        = NULL,
};

const z_ipc_msg_protocol_t r5l_msg_protocol = {
  .msg_alloc   = __client_msg_alloc,
  .msg_parse   = __client_msg_parse,
  .msg_exec    = __client_msg_exec,
  .msg_respond = NULL,
  .msg_free    = NULL,
};
