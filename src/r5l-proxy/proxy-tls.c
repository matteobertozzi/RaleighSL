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

static int __server_setup (z_ipc_server_t *ipc_server) {
  r5l_proxy_t *server = Z_CAST(r5l_proxy_t, ipc_server);
  ipc_server->udata.ptr = NULL;
  server->tls = z_tls_ctx_open();
  z_tls_engine_inhale(server->tls, "cert.pem", "cert.pem");
  return(0);
}

static int __client_connected (z_ipc_client_t *ipc_client) {
  r5l_proxy_client_t *client = Z_CAST(r5l_proxy_client_t, ipc_client);
  r5l_proxy_t *server;

  Z_LOG_INFO("log client connected %p", client);

  server = z_ipc_get_server(r5l_proxy_t, ipc_client);
  client->tls = z_tls_accept(server->tls, Z_IOPOLL_ENTITY_FD(client));
  return(0);
}

static int __client_disconnected (z_ipc_client_t *ipc_client) {
  r5l_proxy_client_t *client = Z_CAST(r5l_proxy_client_t, ipc_client);
  Z_LOG_INFO("log client disconnected %p", client);
  z_tls_disconnect(client->tls);
  return(0);
}

static int __client_read (z_ipc_client_t *ipc_client) {
  r5l_proxy_client_t *client = Z_CAST(r5l_proxy_client_t, ipc_client);
  ssize_t rd;

  rd = z_tls_read(client->tls, client->buffer, sizeof(client->buffer));
  if (rd < 0) {
    perror("read()");
    return(-1);
  }

  client->buffer[rd] = 0;
  printf("read: %ld %s\n", rd, client->buffer);
  client->size = rd;

  z_ipc_client_set_data_available(client, client->size);
  return(0);
}

static int __client_write (z_iopoll_engine_t *iopoll, z_ipc_client_t *ipc_client) {
  r5l_proxy_client_t *client = Z_CAST(r5l_proxy_client_t, ipc_client);
  ssize_t wr;
  wr = z_tls_write(client->tls, client->buffer, client->size);
  printf("write: %ld\n", wr);
  if (wr < 0) {
    perror("write()");
    return(-1);
  }

  client->size -= wr;
  z_ipc_client_set_data_available(client, client->size);
  return(0);
}

const z_ipc_protocol_t r5l_proxy_tls_protocol = {
  /* raw-client protocol */
  .read         = __client_read,
  .write        = __client_write,
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
