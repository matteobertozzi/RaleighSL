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

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <netdb.h>

#include <zcl/string.h>
#include <zcl/socket.h>
#include <zcl/fd.h>

#define __LISTEN_BACKLOG        (16)

/* ===========================================================================
 *  Socket Private Helpers
 */
static int __socket (struct addrinfo *info,
                     int set_reuse_addr,
                     int set_reuse_port)
{
    int s;

    if ((s = socket(info->ai_family, info->ai_socktype, info->ai_protocol)) < 0)
        return(s);

    if (set_reuse_addr) {
        int yep = 1;
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yep, sizeof(int));
    }

    if (set_reuse_port) {
#if defined(SO_REUSEPORT)
        int yep = 1;
        setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &yep, sizeof(int));
#endif /* SO_REUSEPORT */
    }

    return(s);
}

static int __socket_accept (int socket,
                            struct sockaddr *address,
                            socklen_t *address_size,
                            int non_blocking)
{
    int sd;

#if defined(Z_SOCKET_HAS_ACCEPT4)
    if ((sd = accept4(socket, address, address_size,
                      non_blocking ? SOCK_NONBLOCK : 0)) < 0)
    {
        perror("accept4()");
        return(-1);
    }
#else
    if ((sd = accept(socket, (struct sockaddr *)address, address_size)) < 0) {
        perror("accept()");
        return(-1);
    }

    if (non_blocking && z_fd_set_blocking(sd, 0)) {
        close(sd);
        return(-2);
    }
#endif

    return(sd);
}

static int __socket_bind (const char *address,
                          const char *port,
                          struct addrinfo *hints,
                          struct sockaddr_storage *addr)
{
    struct addrinfo *info;
    struct addrinfo *p;
    int status;
    int sock;

    if ((status = getaddrinfo(address, port, hints, &info)) != 0) {
        fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(status));
        return(-1);
    }

    sock = -1;
    for (p = info; p != NULL; p = p->ai_next) {
        if ((sock = __socket(p, 1, 1)) < 0) {
            perror("socket()");
            continue;
        }

        if (bind(sock, info->ai_addr, info->ai_addrlen) < 0) {
            perror("bind()");
            close(sock);
            sock = -1;
            continue;
        }

        if (addr != NULL)
            z_memcpy(addr, p->ai_addr, p->ai_addrlen);

        break;
    }

    freeaddrinfo(info);
    return(sock);
}

/* ===========================================================================
 *  TCP Socket Related
 */
int z_socket_tcp_connect (const char *address,
                          const char *port,
                          struct sockaddr_storage *addr)
{
    struct addrinfo hints;
    struct addrinfo *info;
    struct addrinfo *p;
    int status;
    int sock;

    z_memzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(address, port, &hints, &info)) != 0) {
        fprintf(stderr, "getaddrinfo(): %s", gai_strerror(status));
        return(-1);
    }

    sock = -1;
    for (p = info; p != NULL; p = p->ai_next) {
        if ((sock = __socket(p, 0, 0)) < 0) {
            perror("socket()");
            continue;
        }

        if (connect(sock, p->ai_addr, p->ai_addrlen) < 0) {
            perror("connect()");
            close(sock);
            sock = -1;
            continue;
        }

        if (addr != NULL)
            z_memcpy(addr, p->ai_addr, p->ai_addrlen);

        break;
    }

    freeaddrinfo(info);
    return(sock);
}

int z_socket_tcp_bind (const char *address,
                       const char *port,
                       struct sockaddr_storage *addr)
{
    struct addrinfo hints;
    int sock;

    z_memzero(&hints, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if ((sock = __socket_bind(address, port, &hints, addr)) < 0)
        return(sock);

    if (listen(sock, __LISTEN_BACKLOG) < 0) {
        perror("listen()");
        close(sock);
        return(-2);
    }

    return(sock);
}

int z_socket_tcp_accept (int socket,
                         struct sockaddr_storage *addr,
                         int non_blocking)
{
    struct sockaddr_storage address;
    socklen_t addrsz;
    int sd;

    addrsz = sizeof(struct sockaddr_storage);
    if ((sd = __socket_accept(socket, (struct sockaddr *)&address,
                              &addrsz, non_blocking)) < 0)
    {
        return(-1);
    }

    if (addr != NULL)
        z_memcpy(addr, &address, sizeof(struct sockaddr_storage));

    return(sd);
}

int z_socket_tcp_set_nodelay (int socket) {
    int yes = 1;
    if (setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes)) == -1)
        return(1);
    return(0);
}

/* ===========================================================================
 *  UDP Socket Related
 */
int z_socket_udp_bind (const char *address,
                       const char *port,
                       struct sockaddr_storage *addr)
{
    struct addrinfo hints;

    z_memzero(&hints, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    return(__socket_bind(address, port, &hints, addr));
}

int z_socket_udp_connect (const char *address,
                        const char *port,
                        struct sockaddr_storage *addr)
{
    struct addrinfo hints;
    struct addrinfo *info;
    struct addrinfo *p;
    int status;
    int sock;

    z_memzero(&hints, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    if ((status = getaddrinfo(address, port, &hints, &info)) != 0) {
        fprintf(stderr, "getaddrinfo(): %s", gai_strerror(status));
        return(-1);
    }

    sock = -1;
    for (p = info; p != NULL; p = p->ai_next) {
        if ((sock = __socket(p, 0, 0)) < 0) {
            perror("socket()");
            continue;
        }

        if (addr != NULL)
            z_memcpy(addr, p->ai_addr, p->ai_addrlen);

        break;
    }

    freeaddrinfo(info);
    return(sock);
}

int z_socket_udp_send (int sock,
                       const struct sockaddr_storage *addr,
                       const void *buffer,
                       int n,
                       int flags)
{
    return(sendto(sock, buffer, n, flags,
                  (const struct sockaddr *)addr,
                  sizeof(struct sockaddr_storage)));
}

int z_socket_udp_recv (int sock,
                       struct sockaddr_storage *addr,
                       void *buffer,
                       int n,
                       int flags)
{
    socklen_t addr_size;
    addr_size = sizeof(struct sockaddr_storage);
    return(recvfrom(sock, buffer, n, flags,
                    (struct sockaddr *)addr,
                    &addr_size));
}

/* ===========================================================================
 *  UNIX Socket Related
 */
#ifdef Z_SOCKET_HAS_UNIX

#include <sys/un.h>

int z_socket_unix_connect (const char *filepath) {
    struct sockaddr_un addr;
    socklen_t addrlen;
    int sock;

    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return(-1);
    }

    addr.sun_family = AF_UNIX;
    z_strlcpy(addr.sun_path, filepath, sizeof(addr.sun_path));
    addrlen = (z_strlen(addr.sun_path) + (sizeof(addr)-sizeof(addr.sun_path)));
    if (connect(sock, (struct sockaddr *)&addr, addrlen) < 0) {
        perror("connect()");
        close(sock);
        return(-2);
    }

    return(sock);
}

int z_socket_unix_bind (const char *filepath, int dgram) {
    struct sockaddr_un addr;
    socklen_t addrlen;
    int sock;

    if ((sock = socket(AF_UNIX, dgram ? SOCK_DGRAM : SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return(-2);
    }

    addr.sun_family = AF_UNIX;
    z_strlcpy(addr.sun_path, filepath, sizeof(addr.sun_path));

    addrlen = (z_strlen(addr.sun_path) + (sizeof(addr)-sizeof(addr.sun_path)));
    if (bind(sock, (struct sockaddr *)&addr, addrlen) < 0) {
        perror("bind()");
        close(sock);
        return(-3);
    }

    if (!dgram) {
        if (listen(sock, __LISTEN_BACKLOG) < 0) {
            perror("listen()");
            unlink(addr.sun_path);
            close(sock);
            return(-4);
        }
    }

    return(sock);
}

int z_socket_unix_accept (int socket, int non_blocking) {
    struct sockaddr_un address;
    socklen_t addrsz;
    int sd;

    addrsz = sizeof(struct sockaddr_un);
    if ((sd = __socket_accept(socket, (struct sockaddr *)&address,
                              &addrsz, non_blocking)) < 0)
    {
        return(-1);
    }

    return(sd);
}

int z_socket_unix_sendfd (int sock, int fd) {
    unsigned char cmsgbuf[CMSG_LEN(sizeof(int))];
    struct cmsghdr *cmsg;
    unsigned int magic;
    struct msghdr msg;
    struct iovec iov;

    magic = 0xFD5;
    iov.iov_base = &magic;
    iov.iov_len = sizeof(unsigned int);

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    cmsg = (struct cmsghdr *)cmsgbuf;
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    msg.msg_control = cmsg;
    msg.msg_controllen = cmsg->cmsg_len;
    *(int *)CMSG_DATA(cmsg) = fd;

    if (sendmsg(sock, &msg, 0) != sizeof(unsigned int)) {
        perror("sendmsg()");
        return(-1);
    }

    return(0);
}

int z_socket_unix_recvfd (int sock) {
    unsigned char cmsgbuf[CMSG_LEN(sizeof(int))];
    struct cmsghdr *cmsg;
    struct msghdr msg;
    struct iovec iov;
    unsigned int magic;

    iov.iov_base = &magic;
    iov.iov_len = sizeof(unsigned int);

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    cmsg = (struct cmsghdr *)cmsgbuf;
    msg.msg_control = cmsg;
    msg.msg_controllen = CMSG_LEN(sizeof(int));

    if (recvmsg(sock, &msg, 0) != sizeof(unsigned int)) {
        perror("recvmsg()");
        return(-1);
    }

    if (magic != 0xFD5)
        return(-2);

    return(*(int *)CMSG_DATA(cmsg));
}

int z_socket_unix_send (int sock,
                        const struct sockaddr_un *addr,
                        const void *buffer,
                        int n,
                        int flags)
{
    return(sendto(sock, buffer, n, flags,
                  (const struct sockaddr *)addr,
                  sizeof(struct sockaddr_un)));
}

int z_socket_unix_recv (int sock,
                        struct sockaddr_un *addr,
                        void *buffer,
                        int n,
                        int flags)
{
    socklen_t addr_size;
    addr_size = sizeof(struct sockaddr_un);
    return(recvfrom(sock, buffer, n, flags,
                    (struct sockaddr *)addr,
                    &addr_size));
}

#endif /* Z_SOCKET_HAS_UNIX */

/* ===========================================================================
 *  Socket Related
 */
int z_socket_set_sendbuf (int sock, unsigned int bufsize) {
    return(setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize)) < 0);
}

int z_socket_set_recvbuf (int sock, unsigned int bufsize) {
    return(setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize)) < 0);
}

/* ===========================================================================
 *  Socket Address Related
 */
int z_socket_address (int sock,
                      struct sockaddr_storage *address)
{
    socklen_t addr_size;
    addr_size = sizeof(struct sockaddr_storage);
    return(getpeername(sock, (struct sockaddr *)address, &addr_size));
}

char *z_socket_str_address (char *buffer,
                            unsigned int n,
                            const struct sockaddr *address)
{
    const void *addr;

    if (address->sa_family == AF_INET)
        addr = &((const struct sockaddr_in *)address)->sin_addr;
    else
        addr = &((const struct sockaddr_in6 *)address)->sin6_addr;

    if (inet_ntop(address->sa_family, addr, buffer, n) == NULL)
        return(NULL);

    return(buffer);
}

char *z_socket_str_address_info (char *buffer,
                                 unsigned int n,
                                 const struct addrinfo *info)
{
    const void *addr;

    if (info->ai_family == AF_INET)
        addr = &((const struct sockaddr_in *)info->ai_addr)->sin_addr;
    else
        addr = &((const struct sockaddr_in6 *)info->ai_addr)->sin6_addr;

    if (inet_ntop(info->ai_family, addr, buffer, n) == NULL)
        return(NULL);

    return(buffer);
}

int z_socket_address_is_ipv6 (const struct sockaddr *address) {
    return(address->sa_family != AF_INET);
}
