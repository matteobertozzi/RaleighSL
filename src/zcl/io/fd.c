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
#include <zcl/fd.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#if defined(Z_IOPOLL_HAS_POLL)
  #include <poll.h>
#elif defined(Z_IOPOLL_HAS_SELECT)
  #include <sys/select.h>
#else
  #error "no poll() or select() call available"
#endif

int z_fd_set_blocking (int fd, int blocking) {
  int flags;

  if (Z_UNLIKELY((flags = fcntl(fd, F_GETFL)) < 0)) {
    Z_LOG_ERRNO_WARN("failed fcntl(F_GETFL %d) for fd %d", blocking, fd);
    return(1);
  }

  if (Z_UNLIKELY(blocking)) {
    flags &= ~O_NONBLOCK;
  } else {
    flags |= O_NONBLOCK;
  }

  if (Z_UNLIKELY(fcntl(fd, F_SETFL, flags) < 0)) {
    Z_LOG_ERRNO_WARN("failed fcntl(F_SETFL %d) for fd %d", blocking, fd);
    return(2);
  }

  return(0);
}

int z_fd_wait_readable (int fd, int timeout_msec) {
#if defined(Z_IOPOLL_HAS_POLL)
  struct pollfd ev;
  ev.fd = fd;
  ev.events  = POLLIN | POLLERR | POLLHUP;
  ev.revents = 0;
  if (poll(&ev, 1, timeout_msec) <= 0)
    return(-1);
  return(ev.revents != POLLIN);
#elif defined(Z_IOPOLL_HAS_SELECT)
  struct timeval tv;
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(fd, &rfds);
  tv.tv_sec  = timeout_msec / 1000;
  tv.tv_usec = timeout_msec % 1000;
  if (select(fd + 1, &rfds, NULL, NULL, &tv) <= 0)
    return(-1);
  return(!FD_ISSET(fd, &rfds));
#endif
}

int z_fd_read_avail (int fd, int *avail) {
  return ioctl(fd, FIONREAD, avail) < 0;
}

ssize_t z_fd_read (int fd, void *buf, size_t bufsize) {
  ssize_t rd = read(fd, buf, bufsize);
  if (Z_LIKELY(rd >= 0)) return(rd);
  return (errno == EAGAIN || errno == EWOULDBLOCK) ? 0 : rd;
}

ssize_t z_fd_readv (int fd, const struct iovec *iov, int iovcnt) {
  ssize_t rd = readv(fd, iov, iovcnt);
  if (Z_LIKELY(rd >= 0)) return(rd);
  return (errno == EAGAIN || errno == EWOULDBLOCK) ? 0 : rd;
}

ssize_t z_fd_preadv (int fd, uint64_t offset, const struct iovec *iov, int iovcnt) {
#if defined(Z_IO_HAS_PWRITEV)
  ssize_t rd = preadv(fd, iov, iovcnt, offset);
  if (Z_LIKELY(rd > 0)) return(rd);
  return (errno == EAGAIN || errno == EWOULDBLOCK) ? 0 : rd;
#else
  if (Z_UNLIKELY(lseek(fd, offset, SEEK_SET) != offset))
    return(-1);
  return(z_fd_readv(fd, iov, iovcnt));
#endif
}

ssize_t z_fd_skip (int fd, size_t length) {
  uint8_t buffer[2048];
  ssize_t rdtotal = 0;
  while (length >= sizeof(buffer)) {
    ssize_t rd = read(fd, buffer, sizeof(buffer));
    if (Z_UNLIKELY(rd < 0))
      return (errno == EAGAIN || errno == EWOULDBLOCK) ? 0 : rd;
    rdtotal += rd;
  }
  if (length > 0) {
    ssize_t rd = read(fd, buffer, length);
    if (Z_UNLIKELY(rd < 0))
      return (errno == EAGAIN || errno == EWOULDBLOCK) ? 0 : rd;
    rdtotal += rd;
  }
  return(rdtotal);
}

ssize_t z_fd_write (int fd, const void *buf, size_t bufsize) {
  const char *p = buf;
  ssize_t total = 0;

  while (bufsize > 0) {
    ssize_t written = write(fd, p, bufsize);
    if (written <= 0 || errno == EAGAIN || errno == EWOULDBLOCK)
      return(written);

    p += written;
    total += written;
    bufsize -= written;
  }

  return(total);
}

ssize_t z_fd_writev(int fd, const struct iovec *iov, int iovcnt) {
  ssize_t wr = writev(fd, iov, iovcnt);
  if (Z_LIKELY(wr >= 0)) return wr;
  return (errno == EAGAIN || errno == EWOULDBLOCK) ? 0 : wr;
}

ssize_t z_fd_pwritev (int fd, uint64_t offset, const struct iovec *iov, int iovcnt) {
#if defined(Z_IO_HAS_PWRITEV)
  ssize_t wr = pwritev(fd, iov, iovcnt, offset);
  if (Z_LIKELY(wr >= 0)) return wr;
  return (errno == EAGAIN || errno == EWOULDBLOCK) ? 0 : wr;
#else
  if (Z_UNLIKELY(lseek(fd, offset, SEEK_SET) != offset))
    return(-1);
  return(z_fd_writev(fd, iov, iovcnt));
#endif
}
