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
 #include <zcl/debug.h>
#include <zcl/iovec.h>

/* ===========================================================================
 *  INTERFACE Reader methods
 */
static size_t __iovec_reader_next (void *self, const uint8_t **data) {
  const struct iovec *iov = Z_READER_READABLE(struct iovec, self);
  z_iovec_reader_t *reader = Z_IOVEC_READER(self);
  size_t chunk;

  if (!reader->avail)
    return(0);

  iov += reader->ivec;
  chunk = iov[0].iov_len - reader->offset;
  if (chunk > 0) {
    *data = Z_UINT8_PTR(iov[0].iov_base) + reader->offset;
    reader->offset += chunk;
    reader->avail -= chunk;
    return(chunk);
  }

  ++iov;
  ++(reader->ivec);
  *data = Z_UINT8_PTR(iov[0].iov_base);
  reader->offset = iov[0].iov_len;
  reader->avail -= iov[0].iov_len;
  return(iov[0].iov_len);
}

static void __iovec_reader_backup (void *self, size_t count) {
  z_iovec_reader_t *reader = Z_IOVEC_READER(self);
  reader->offset -= count;
  reader->avail += count;
}

static size_t __iovec_reader_available (void *self) {
  return(Z_IOVEC_READER(self)->avail);
}

const uint8_t *__iovec_reader_fetch (void *self, uint8_t *buffer, size_t length) {
  const struct iovec *iov = Z_READER_READABLE(struct iovec, self);
  z_iovec_reader_t *reader = Z_IOVEC_READER(self);
  uint8_t *pbuf;
  size_t avail;

  if (reader->avail < length)
    return(NULL);

  iov += reader->ivec;
  if ((avail = (iov[0].iov_len - reader->offset)) == 0) {
    reader->offset = 0;
    ++(reader->ivec);
    ++iov;
    avail = iov[0].iov_len;
  }

  if (length <= avail) {
    pbuf = Z_UINT8_PTR(iov[0].iov_base) + reader->offset;
    reader->offset += length;
    reader->avail -= length;
    return(pbuf);
  }

  reader->avail -= length;
  pbuf = buffer;

  /* Read first chunk */
  z_memcpy(pbuf, Z_UINT8_PTR(iov[0].iov_base) + reader->offset, avail);
  length -= avail;
  pbuf += avail;
  ++(reader->ivec);
  ++iov;

  /* Read full middle chunks */
  while (length >= iov[0].iov_len) {
    z_memcpy(pbuf, iov[0].iov_base, iov[0].iov_len);
    length -= iov[0].iov_len;
    pbuf += iov[0].iov_len;
    ++(reader->ivec);
    ++iov;
  }

  /* Read last chunk */
  reader->offset = length;
  if (length > 0) {
    z_memcpy(pbuf, iov[0].iov_base, length);
  }

  return buffer;
}

/* ===========================================================================
 *  Slice vtables
 */
static const z_vtable_reader_t __iovec_reader = {
  .open       = NULL,
  .close      = NULL,
  .next       = __iovec_reader_next,
  .backup     = __iovec_reader_backup,
  .available  = __iovec_reader_available,
  .fetch      = __iovec_reader_fetch,
};

/* ===========================================================================
*  PUBLIC Reader constructor/destructor
*/
int z_iovec_reader_open (z_iovec_reader_t *reader, const struct iovec *iov, int nvec) {
  int i;
  Z_VREADER_INIT(reader, &__iovec_reader, iov);
  reader->ivec = 0;
  reader->offset = 0;
  reader->avail = 0;
  for (i = 0; i < nvec; ++i) {
    reader->avail += iov[i].iov_len;
  }
  return(0);
}

void z_iovec_reader_close (z_iovec_reader_t *reader) {
}
