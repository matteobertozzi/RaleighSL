/*
 *   Copyright 2011-2013 Matteo Bertozzi
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

/* ============================================================================
 *  IPC over TPC socket
 */
int z_ipc_bind_tcp (const void *hostname, const void *service) {
    int sock;

    if ((sock = z_socket_tcp_bind(hostname, service, NULL)) < 0)
        return(-1);

    Z_LOG_INFO("Service %d up and running on %s:%s...\n", sock, hostname, service);
    return(sock);
}

int z_ipc_accept_tcp (z_ipc_server_t *server) {
    struct sockaddr_storage address;
    char ip[Z_INET6_ADDRSTRLEN];
    int csock;

    if ((csock = z_socket_tcp_accept(Z_IOPOLL_ENTITY_FD(server), &address, 1)) < 0)
        return(-1);

    z_socket_tcp_set_nodelay(csock);
    z_socket_str_address(ip, Z_INET6_ADDRSTRLEN, (struct sockaddr *)&address);
    Z_LOG_INFO("Service %d accepted %d %s\n", Z_IOPOLL_ENTITY_FD(server), csock, ip);
    return(csock);
}

/* ============================================================================
 *  IPC over Unix socket
 */
#ifdef Z_SOCKET_HAS_UNIX
int z_ipc_bind_unix (const void *path, const void *service) {
    int sock;

    if ((sock = z_socket_unix_bind((const char *)path, 0)) < 0)
        return(-1);

    Z_LOG_INFO("Service %d up and running on %s...\n", sock, path);
    return(sock);
}

int z_ipc_accept_unix (z_ipc_server_t *server) {
    int csock;

    if ((csock = z_socket_unix_accept(Z_IOPOLL_ENTITY_FD(server), 1)) < 0)
        return(-1);

    Z_LOG_INFO("Service %d accepted %d\n", Z_IOPOLL_ENTITY_FD(server), csock);
    return(csock);
}
#endif /* Z_SOCKET_HAS_UNIX */
