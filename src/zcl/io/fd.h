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

#ifndef _Z_FD_H_
#define _Z_FD_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

int     z_fd_set_blocking (int fd, int blocking);
int     z_fd_get_path     (int fd, char *path, int size);

ssize_t z_fd_write        (int fd, const void *buf, size_t bufsize);
ssize_t z_fd_writev       (int fd, const struct iovec *iov, int iovcnt);

#define z_fd_read         read
#define z_fd_readv        readv

ssize_t z_fd_preadv  (int fd, uint64_t offset, const struct iovec *iov, int iovcnt);
ssize_t z_fd_pwritev (int fd, uint64_t offset, const struct iovec *iov, int iovcnt);

__Z_END_DECLS__

#endif /* _Z_FD_H_ */
