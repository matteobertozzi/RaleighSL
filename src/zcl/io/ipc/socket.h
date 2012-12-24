/*
 *   Copyright 2011-2012 Matteo Bertozzi
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

#ifndef _Z_SOCKET_H_
#define _Z_SOCKET_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#define Z_INET6_ADDRSTRLEN           46

#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

int     z_socket_tcp_connect      (const char *address,
                                   const char *port,
                                   struct sockaddr_storage *addr);
int     z_socket_tcp_bind         (const char *address,
                                   const char *port,
                                   struct sockaddr_storage *addr);
int     z_socket_tcp_accept       (int socket,
                                   struct sockaddr_storage *addr,
                                   int non_blocking);
int     z_socket_tcp_set_nodelay  (int socket);


int     z_socket_udp_bind         (const char *address,
                                   const char *port,
                                   struct sockaddr_storage *addr);
int     z_socket_udp_connect      (const char *address,
                                   const char *port,
                                   struct sockaddr_storage *addr);
int     z_socket_udp_send         (int sock,
                                   const struct sockaddr_storage *addr,
                                   const void *buffer,
                                   int n,
                                   int flags);
int     z_socket_udp_recv         (int sock,
                                   struct sockaddr_storage *addr,
                                   void *buffer,
                                   int n,
                                   int flags);

#ifdef Z_SOCKET_HAS_UNIX
struct sockaddr_un;

int     z_socket_unix_connect     (const char *filepath);
int     z_socket_unix_bind        (const char *filepath, int dgram);
int     z_socket_unix_accept      (int socket, int non_blocking);

int     z_socket_unix_sendfd      (int sock, int fd);
int     z_socket_unix_recvfd      (int sock);

int     z_socket_unix_send        (int sock,
                                   const struct sockaddr_un *addr,
                                   const void *buffer,
                                   int n,
                                   int flags);
int     z_socket_unix_recv        (int sock,
                                   struct sockaddr_un *addr,
                                   void *buffer,
                                   int n,
                                   int flags);
#endif /* Z_SOCKET_HAS_UNIX */

int     z_socket_set_sendbuf      (int sock,
                                   unsigned int bufsize);
int     z_socket_set_recvbuf      (int sock,
                                   unsigned int bufsize);

int     z_socket_address          (int sock,
                                   struct sockaddr_storage *addr);

char *  z_socket_str_address      (char *buffer,
                                   unsigned int n,
                                   const struct sockaddr *address);
char *  z_socket_str_address_info (char *buffer,
                                   unsigned int n,
                                   const struct addrinfo *address);

int     z_socket_address_is_ipv6  (const struct sockaddr *address);

__Z_END_DECLS__

#endif /* !_Z_SOCKET_H_ */
