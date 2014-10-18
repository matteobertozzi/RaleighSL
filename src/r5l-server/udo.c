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

#include <zcl/socket.h>
#include <zcl/debug.h>
#include <zcl/ipc.h>

#include "server.h"

static int __udo_server_read (z_ipc_server_t *server) {
  struct sockaddr_storage addr;
  uint8_t buffer[128];
  char addr_str[64];
  int n;

  n = z_socket_udp_recv(Z_IOPOLL_ENTITY_FD(server), &addr, buffer, sizeof(buffer), 0);
  if (Z_UNLIKELY(n < 0)) {
    perror("udp_recv()");
    return(0);
  }

  buffer[n] = 0;
  z_socket_str_address(addr_str, sizeof(addr_str), (struct sockaddr *)&addr);
  Z_LOG_DEBUG("GOT n %dbytes from %s: %s", n, addr_str, buffer);
  return(0);
}

const z_ipc_protocol_t udo_udp_protocol = {
  /* raw-client protocol */
  .read         = NULL,
  .write        = NULL,
  /* client protocol */
  .connected    = NULL,
  .disconnected = NULL,

  /* server protocol */
  .uevent       = NULL,
  .timeout      = NULL,
  .accept       = __udo_server_read,
  .bind         = z_ipc_bind_udp,
  .unbind       = NULL,
  .setup        = NULL,
};
