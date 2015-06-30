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

#include <zcl/debug.h>
#include <zcl/time.h>
#include <zcl/fd.h>

#include "proxy.h"

static int __server_setup (z_ipc_server_t *server) {
  server->udata.ptr = NULL;
  return(0);
}

static int __client_connected (z_ipc_client_t *ipc_client) {
  r5l_proxy_client_t *client = Z_CAST(r5l_proxy_client_t, ipc_client);
  Z_LOG_INFO("log client connected %p", client);
  return(0);
}

static int __client_disconnected (z_ipc_client_t *ipc_client) {
  r5l_proxy_client_t *client = Z_CAST(r5l_proxy_client_t, ipc_client);
  Z_LOG_INFO("log client disconnected %p", client);
  return(0);
}

const z_ipc_protocol_t r5l_proxy_tcp_protocol = {
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
  .setup        = __server_setup,
};

const z_ipc_protocol_t r5l_proxy_unix_protocol = {
  /* raw-client protocol */
  .read         = NULL,
  .write        = NULL,
  /* client protocol */
  .connected    = __client_connected,
  .disconnected = __client_disconnected,

  /* server protocol */
  .uevent       = NULL,
  .timeout      = NULL,
  .accept       = z_ipc_accept_unix,
  .bind         = z_ipc_bind_unix,
  .unbind       = z_ipc_unbind_unix,
  .setup        = __server_setup,
};
