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

#ifndef _Z_IOVEC_H_
#define _Z_IOVEC_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/reader.h>

#define Z_IOVEC_READER(x)               Z_CAST(z_iovec_reader_t, x)

Z_TYPEDEF_STRUCT(z_iovec_reader);

struct z_iovec_reader {
  __Z_READABLE__
  int ivec;
  size_t offset;
  size_t avail;
};

int  z_iovec_reader_open  (z_iovec_reader_t *reader, const struct iovec *iov, int nvec);
void z_iovec_reader_close (z_iovec_reader_t *reader);

__Z_END_DECLS__

#endif /* _Z_IOVEC_H_ */