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

#ifndef _Z_IO_H_
#define _Z_IO_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

typedef struct z_io_seq_vtable z_io_seq_vtable_t;
typedef struct z_io_buf z_io_buf_t;

struct z_io_seq_vtable {
  ssize_t (*read)   (void *udata, void *buffer, size_t bufsize);
  ssize_t (*write)  (void *udata, const void *buffer, size_t bufsize);

  ssize_t (*readv)  (void *udata, const struct iovec *iov, int iovcnt);
  ssize_t (*writev) (void *udata, const struct iovec *iov, int iovcnt);
};

struct z_io_buf {
  const uint8_t *buffer;
  size_t   buflen;
};

extern const z_io_seq_vtable_t z_io_buf_vtable;

__Z_END_DECLS__

#endif /* !_Z_IO_H_ */
