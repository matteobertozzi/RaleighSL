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

#include <zcl/string.h>
#include <zcl/io.h>
#include <zcl/fd.h>

/* ============================================================================
 *  I/O Buffer
 */
static ssize_t __io_buf_readv(void *udata, const struct iovec *iov, int iovcnt) {
  z_io_buf_t *self = Z_CAST(z_io_buf_t, udata);
  ssize_t rd = 0;
  for (; iovcnt-- > 0; ++iov) {
    size_t n = z_min(self->buflen, iov->iov_len);
    memcpy(iov->iov_base, self->buffer, n);
    self->buffer += n;
    self->buflen -= n;
    rd += n;
  }
  return(rd);
}

const z_io_seq_vtable_t z_io_buf_vtable = {
  .read   = NULL,
  .write  = NULL,
  .readv  = __io_buf_readv,
  .writev = NULL,
};
