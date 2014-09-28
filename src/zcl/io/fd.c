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
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

int z_fd_set_blocking (int fd, int blocking) {
  int flags;

  if ((flags = fcntl(fd, F_GETFL)) < 0) {
    perror("fcntl()");
    return(1);
  }

  if (blocking)
    flags &= ~O_NONBLOCK;
  else
    flags |= O_NONBLOCK;

  if (fcntl(fd, F_SETFL, flags) < 0) {
    perror("fcntl()");
    return(2);
  }

  return(0);
}

static int __fd_read_link (int fd, char *path, int size) {
  char fd_path[255];
  snprintf(fd_path, sizeof(fd_path), "/proc/self/fd/%d", fd);
  return(readlink(fd_path, path, size) < 0);
}

int z_fd_get_path (int fd, char *path, int size) {
#if defined(F_GETPATH)
  if (fcntl(fd, F_GETPATH, path) > 0)
    return(0);
#endif
  return(__fd_read_link(fd, path, size));
}

ssize_t z_fd_write (int fd, const void *buf, size_t bufsize) {
  const char *p = buf;
  ssize_t total = 0;

  while (bufsize > 0) {
    ssize_t written = write(fd, p, bufsize);
    if (written <= 0)
      return(written);

    p += written;
    total += written;
    bufsize -= written;
  }
  return(total);
}

ssize_t z_fd_writev(int fd, const struct iovec *iov, int iovcnt) {
  ssize_t wr = writev(fd, iov, iovcnt);
  /* TODO */
#if 0 && __Z_DEBUG__
  ssize_t total = 0;

  while (iovcnt--) {
    total += iov->iov_len;
    iov++;
  }

  if (wr >= 0) {
    Z_ASSERT(wr == total, "Incomplete writev wr=%zd total=%zd", wr, total);
  }
#endif
  return(wr);
}

ssize_t z_fd_read (int fd, void *buf, size_t bufsize) {
  return read(fd, buf, bufsize);
}

ssize_t z_fd_readv (int fd, const struct iovec *iov, int iovcnt) {
  return readv(fd, iov, iovcnt);
}

ssize_t z_fd_preadv (int fd, uint64_t offset, const struct iovec *iov, int iovcnt) {
#if defined(Z_IO_HAS_PWRITEV)
  /* TODO */
  lseek(fd, offset, SEEK_SET);
  return(z_fd_readv(fd, iov, iovcnt));
#else
  lseek(fd, offset, SEEK_SET);
  return(z_fd_readv(fd, iov, iovcnt));
#endif
}

ssize_t z_fd_pwritev (int fd, uint64_t offset, const struct iovec *iov, int iovcnt) {
#if defined(Z_IO_HAS_PWRITEV)
  /* TODO */
  lseek(fd, offset, SEEK_SET);
  return(z_fd_writev(fd, iov, iovcnt));
#else
  lseek(fd, offset, SEEK_SET);
  return(z_fd_writev(fd, iov, iovcnt));
#endif
}
