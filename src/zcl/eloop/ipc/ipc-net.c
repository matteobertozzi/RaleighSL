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

#include <zcl/socket.h>
#include <zcl/debug.h>
#include <zcl/ipc.h>
#include <zcl/fd.h>

/* ============================================================================
 *  IPC over TPC socket
 */
int z_ipc_bind_tcp (z_ipc_server_t *server,
                    const void *hostname,
                    const void *service)
{
  int sock;

  sock = z_socket_tcp_bind(hostname, service, NULL);
  if (Z_UNLIKELY(sock < 0))
    return(-1);

  return(sock);
}

int z_ipc_accept_tcp (z_ipc_server_t *server) {
  struct sockaddr_storage address;
  int csock;

  csock = z_socket_tcp_accept(Z_IOPOLL_ENTITY_FD(server), &address, 1);
  if (Z_UNLIKELY(csock < 0))
    return(-1);

  z_socket_tcp_set_nodelay(csock);

  if (Z_UNLIKELY(z_log_is_trace_enabled())) {
    char ip[128];
    z_socket_str_address(ip, sizeof(ip), &address);
    Z_LOG_TRACE("Service %s %d accepted %d %s",
                server->name, Z_IOPOLL_ENTITY_FD(server), csock, ip);
  }
  return(csock);
}

/* ============================================================================
 *  IPC over Unix socket
 */
#ifdef Z_SOCKET_HAS_UNIX

int z_ipc_bind_unix (z_ipc_server_t *server,
                     const void *path,
                     const void *service)
{
  int sock;

  sock = z_socket_unix_bind((const char *)path, 0);
  if (Z_UNLIKELY(sock < 0))
    return(-1);

  return(sock);
}

void z_ipc_unbind_unix (z_ipc_server_t *server) {
  char path[4096];
  if (!z_socket_unix_address(Z_IOPOLL_ENTITY_FD(server), path, sizeof(path))) {
    if (unlink(path)) {
      perror("unlink()");
    }
  }
}

int z_ipc_accept_unix (z_ipc_server_t *server) {
  int csock;

  csock = z_socket_unix_accept(Z_IOPOLL_ENTITY_FD(server), 1);
  if (Z_UNLIKELY(csock < 0))
    return(-1);

  return(csock);
}

/* ============================================================================
 *  IPC over UDP socket
 */
int z_ipc_bind_udp (z_ipc_server_t *server,
                    const void *hostname,
                    const void *service)
{
  int sock;

  sock = z_socket_udp_bind(hostname, service, NULL);
  if (Z_UNLIKELY(sock < 0))
    return(-1);

  return(sock);
}

int z_ipc_bind_udp_broadcast (z_ipc_server_t *server,
                              const void *hostname,
                              const void *service)
{
  int sock;

  sock = z_socket_udp_broadcast_bind(hostname, service, NULL);
  if (Z_UNLIKELY(sock < 0))
    return(-1);

  return(sock);
}

#endif /* Z_SOCKET_HAS_UNIX */
