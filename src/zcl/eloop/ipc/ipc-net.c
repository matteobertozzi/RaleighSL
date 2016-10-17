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
  struct sockaddr_storage address;
  int sock;

  sock = z_socket_tcp_bind(hostname, service, &address);
  if (Z_UNLIKELY(sock < 0))
    return(-1);

  if (z_log_is_info_enabled()) {
    char ip[128];
    z_socket_str_address(ip, sizeof(ip), &address);
    Z_LOG_INFO("TCP Service %s %d bind %s:%s (%s)",
               server->name, sock,
               Z_CONST_CAST(char, hostname), Z_CONST_CAST(char, service), ip);
  }
  return(sock);
}

int z_ipc_accept_tcp (z_ipc_server_t *server) {
  struct sockaddr_storage address;
  int csock;

  csock = z_socket_tcp_accept(server->io_entity.fd, &address, 1);
  if (Z_UNLIKELY(csock < 0))
    return(-1);

  z_socket_tcp_set_nodelay(csock);

  if (Z_UNLIKELY(z_log_is_debug_enabled())) {
    char ip[128];
    z_socket_str_address(ip, sizeof(ip), &address);
    Z_LOG_DEBUG("TCP Service %s %d accepted %d %s",
                server->name, server->io_entity.fd, csock, ip);
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

  sock = z_socket_unix_bind(Z_CONST_CAST(char, path), 0);
  if (Z_UNLIKELY(sock < 0))
    return(-1);

  if (z_log_is_info_enabled()) {
    Z_LOG_INFO("Unix Service %s %d bind %s",
               server->name, sock, Z_CONST_CAST(char, path));
  }
  return(sock);
}

void z_ipc_unbind_unix (z_ipc_server_t *server) {
  char path[4096];
  if (!z_socket_unix_address(server->io_entity.fd, path, sizeof(path))) {
    if (unlink(path)) {
      perror("unlink()");
    }
  }
}

int z_ipc_accept_unix (z_ipc_server_t *server) {
  int csock;

  csock = z_socket_unix_accept(server->io_entity.fd, 1);
  if (Z_UNLIKELY(csock < 0))
    return(-1);

  if (Z_UNLIKELY(z_log_is_debug_enabled())) {
    Z_LOG_DEBUG("Unix Service %s %d accepted %d",
                server->name, server->io_entity.fd, csock);
  }
  return(csock);
}

/* ============================================================================
 *  IPC over UDP socket
 */
int z_ipc_bind_udp (z_ipc_server_t *server,
                    const void *hostname,
                    const void *service)
{
  struct sockaddr_storage address;
  int sock;

  sock = z_socket_udp_bind(hostname, service, &address);
  if (Z_UNLIKELY(sock < 0))
    return(-1);

  if (z_log_is_info_enabled()) {
    char ip[128];
    z_socket_str_address(ip, sizeof(ip), &address);
    Z_LOG_INFO("UDP Service %s %d bind %s:%s (%s)",
               server->name, sock,
               Z_CONST_CAST(char, hostname), Z_CONST_CAST(char, service), ip);
  }
  return(sock);
}

int z_ipc_bind_udp_broadcast (z_ipc_server_t *server,
                              const void *hostname,
                              const void *service)
{
  struct sockaddr_storage address;
  int sock;

  sock = z_socket_udp_broadcast_bind(hostname, service, &address);
  if (Z_UNLIKELY(sock < 0))
    return(-1);

  if (z_log_is_info_enabled()) {
    char ip[128];
    z_socket_str_address(ip, sizeof(ip), &address);
    Z_LOG_INFO("UDP Broadcast Service %s %d bind %s:%s (%s)",
               server->name, sock,
               Z_CONST_CAST(char, hostname), Z_CONST_CAST(char, service), ip);
  }
  return(sock);
}

#endif /* Z_SOCKET_HAS_UNIX */
