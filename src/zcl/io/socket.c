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


#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <netdb.h>

#include <zcl/memutil.h>
#include <zcl/endian.h>
#include <zcl/string.h>
#include <zcl/socket.h>
#include <zcl/debug.h>
#include <zcl/fd.h>

#if defined(SOMAXCONN)
  #define __LISTEN_BACKLOG        SOMAXCONN
#else
  #define __LISTEN_BACKLOG        128
#endif

/* ===========================================================================
 *  Socket Private Helpers
 */
static int __socket (struct addrinfo *info,
                     int set_reuse_addr,
                     int set_reuse_port)
{
  int sock;

  sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
  if (Z_UNLIKELY(sock < 0))
    return(sock);

  if (set_reuse_addr) {
    int yep = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yep, sizeof(int));
  }

  if (set_reuse_port) {
#if defined(SO_REUSEPORT)
    int yep = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &yep, sizeof(int));
#endif /* SO_REUSEPORT */
  }

  return(sock);
}

static int __socket_accept (int socket,
                            struct sockaddr *address,
                            socklen_t *address_size,
                            int non_blocking)
{
  int sd;
#if defined(Z_SOCKET_HAS_ACCEPT4)
  sd = accept4(socket, address, address_size, non_blocking ? SOCK_NONBLOCK : 0);
  if (Z_UNLIKELY(sd < 0)) {
    Z_LOG_ERRNO_WARN("accept4() sock %d", socket);
    return(-1);
  }
#else
  sd = accept(socket, (struct sockaddr *)address, address_size);
  if (Z_UNLIKELY(sd < 0)) {
    Z_LOG_ERRNO_WARN("accept() sock %d", socket);
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
                          const char *service,
                          struct addrinfo *hints,
                          struct sockaddr_storage *addr)
{
  struct addrinfo *info;
  struct addrinfo *p;
  int status;
  int sock;

  if ((status = getaddrinfo(address, service, hints, &info)) != 0) {
    Z_LOG_WARN("getaddrinfo(): %s\n", gai_strerror(status));
    return(-1);
  }

  sock = -1;
  for (p = info; p != NULL; p = p->ai_next) {
    if ((sock = __socket(p, 1, 1)) < 0) {
      Z_LOG_ERRNO_WARN("socket()");
      continue;
    }

    if (bind(sock, p->ai_addr, p->ai_addrlen) < 0) {
      Z_LOG_ERRNO_WARN("bind()");
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
    Z_LOG_WARN("getaddrinfo(): %s\n", gai_strerror(status));
    return(-1);
  }

  sock = -1;
  for (p = info; p != NULL; p = p->ai_next) {
    if ((sock = __socket(p, 0, 0)) < 0) {
      Z_LOG_ERRNO_WARN("socket()");
      continue;
    }

    if (connect(sock, p->ai_addr, p->ai_addrlen) < 0) {
      close(sock);
      sock = -1;
      continue;
    }

    if (addr != NULL)
      z_memcpy(addr, p->ai_addr, p->ai_addrlen);
    break;
  }

  if (sock < 0) {
    Z_LOG_ERRNO_WARN("connect()");
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
    Z_LOG_ERRNO_WARN("listen()");
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
  sd = __socket_accept(socket, (struct sockaddr *)&address, &addrsz, non_blocking);
  if (Z_UNLIKELY(sd < 0)) {
    return(-1);
  }

  if (addr != NULL)
    z_memcpy(addr, &address, sizeof(struct sockaddr_storage));
  return(sd);
}

int z_socket_tcp_set_nodelay (int socket) {
  int r, yes = 1;
  r = setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));
  return(r == -1);
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

int z_socket_udp_broadcast_bind (const char *address,
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

  if ((status = getaddrinfo(address, port, &hints, &info)) != 0) {
    Z_LOG_WARN("getaddrinfo(): %s\n", gai_strerror(status));
    return(-1);
  }

  sock = -1;
  for (p = info; p != NULL; p = p->ai_next) {
    int yep;

    if ((sock = __socket(p, 0, 0)) < 0) {
      Z_LOG_ERRNO_WARN("socket()");
      continue;
    }

    yep = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &yep, sizeof(int)) < 0) {
      Z_LOG_ERRNO_WARN("setsockopt(SO_BROADCAST)");
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
    Z_LOG_WARN("getaddrinfo(): %s", gai_strerror(status));
    return(-1);
  }

  sock = -1;
  for (p = info; p != NULL; p = p->ai_next) {
    if ((sock = __socket(p, 0, 0)) < 0) {
      Z_LOG_ERRNO_WARN("socket()");
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
  socklen_t addr_size;
  switch (addr->ss_family) {
    case AF_INET:  addr_size = sizeof(struct sockaddr_in);  break;
    case AF_INET6: addr_size = sizeof(struct sockaddr_in6); break;
    default: return(-1);
  }
  return(sendto(sock, buffer, n, flags,
                (const struct sockaddr *)addr,
                addr_size));
}

int z_socket_udp_recv (int sock,
                       struct sockaddr_storage *addr,
                       void *buffer,
                       int n,
                       int flags,
                       int timeout_msec)
{
  socklen_t addr_size;

  if (timeout_msec > 0 && z_fd_wait_readable(sock, timeout_msec))
    return(-1);

  addr_size = sizeof(struct sockaddr_storage);
  return(recvfrom(sock, buffer, n, flags,
                  (struct sockaddr *)addr,
                  &addr_size));
}

int z_socket_udp_msend (int sock,
                        struct mmsghdr *msgvec,
                        unsigned int vlen,
                        unsigned int flags)
{
#ifdef Z_SOCKET_HAS_MMSG
  return sendmmsg(sock, msgvec, vlen, flags);
#else
  int ret;

  if ((ret = sendmsg(sock, &(msgvec->msg_hdr), flags)) < 0)
    return(-1);

  msgvec->msg_len = ret;
  return(1);
#endif
}

int z_socket_udp_mrecv (int sock,
                        struct mmsghdr *msgvec,
                        unsigned int vlen,
                        unsigned int flags,
                        int timeout_msec)
{
#ifdef Z_SOCKET_HAS_MMSG
  struct timespec *timeout = NULL;
  if (timeout_msec > 0) {
    struct timespec tbuf;
    tbuf.tv_sec  = timeout_msec / 1000;
    tbuf.tv_nsec = (timeout_msec % 1000) * 1000;
    timeout = &tbuf;
  }
  return recvmmsg(sock, msgvec, vlen, flags, timeout);
#else
  int ret;

  if (timeout_msec > 0 && z_fd_wait_readable(sock, timeout_msec))
    return(-1);

  if ((ret = recvmsg(sock, &(msgvec->msg_hdr), flags)) < 0)
    return(-1);

  msgvec->msg_len = ret;
  return(1);
#endif
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
    Z_LOG_ERRNO_WARN("socket()");
    return(-1);
  }

  addr.sun_family = AF_UNIX;
  z_strlcpy(addr.sun_path, filepath, sizeof(addr.sun_path));
  addrlen = (z_strlen(addr.sun_path) + (sizeof(addr)-sizeof(addr.sun_path)));
  if (connect(sock, (struct sockaddr *)&addr, addrlen) < 0) {
    Z_LOG_ERRNO_WARN("connect()");
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
    Z_LOG_ERRNO_WARN("socket()");
    return(-2);
  }

  addr.sun_family = AF_UNIX;
  z_strlcpy(addr.sun_path, filepath, sizeof(addr.sun_path));

  addrlen = (z_strlen(addr.sun_path) + (sizeof(addr)-sizeof(addr.sun_path)));
  if (bind(sock, (struct sockaddr *)&addr, addrlen) < 0) {
    Z_LOG_ERRNO_WARN("bind()");
    close(sock);
    return(-3);
  }

  if (!dgram) {
    if (listen(sock, __LISTEN_BACKLOG) < 0) {
      Z_LOG_ERRNO_WARN("listen()");
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
    Z_LOG_ERRNO_WARN("sendmsg() sock %d", sock);
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
    Z_LOG_ERRNO_WARN("recvmsg() sock %d", sock);
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

int z_socket_bind_address (int sock,
                           struct sockaddr_storage *address)
{
  socklen_t addr_size;
  addr_size = sizeof(struct sockaddr_storage);
  return(getsockname(sock, (struct sockaddr *)address, &addr_size));
}

int z_socket_port (int sock) {
  struct sockaddr_storage addr;
  if (Z_UNLIKELY(z_socket_address(sock, &addr) < 0))
    return(-1);

  switch (addr.ss_family) {
    case AF_INET:
      return ntohs(((struct sockaddr_in *)&addr)->sin_port);
    case AF_INET6:
      return ntohs(((struct sockaddr_in6 *)&addr)->sin6_port);
  }
  return(-1);
}

char *z_socket_str_address (char *buffer,
                            unsigned int n,
                            const struct sockaddr_storage *address)
{
  const void *addr;

  if (address->ss_family == AF_INET) {
    addr = &((const struct sockaddr_in *)address)->sin_addr;
  } else {
    addr = &((const struct sockaddr_in6 *)address)->sin6_addr;
  }

  if (inet_ntop(address->ss_family, addr, buffer, n) == NULL)
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
