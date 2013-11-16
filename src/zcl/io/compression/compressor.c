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

#include <zcl/compression.h>
#include <zcl/string.h>
#include <zcl/coding.h>
#include <zcl/debug.h>

#define __SAFETY_THRESHOLD(size)        ((size >> 8) + 16)

/* ============================================================================
 *  PRIVATE Compressor methods
 */
static int __compressor_flush_buffer (z_compressor_t *self, const uint8_t *data, size_t size) {
  uint8_t *dst = self->dst_buffer + (self->dst_size - self->dst_avail);
  unsigned int size_len;
  size_t data_len;

  Z_ASSERT(size > 0, "Flush size must be not zero: %zu", size);

  /* write uncompressed block size */
  size_len = z_encode_vint(dst, size);
  dst += size_len;

  /* write compressed data */
  data_len = self->codec(dst, self->dst_avail - size_len, data, size);
  self->dst_avail -= (data_len > 0) ? (size_len + data_len) : 0;

  self->bufavail = self->bufsize;
  z_byte_slice_clear(&(self->bprev));
  self->iprev = 0;
  return(data_len);
}

static int __compressor_emergency_flush (z_compressor_t *self, size_t size) {
  size_t n = (self->bufsize - self->bufavail);
  /* safety flush if we're low on buffer */
  if (self->dst_avail < (n + size + __SAFETY_THRESHOLD(size))) {
    if (!__compressor_flush_buffer(self, self->buffer, n)) {
      /* TODO: This should never fail */
      return(1);
    }

    if (self->dst_avail < z_vint_size(size))
      return(2);
  }
  return(0);
}

static int __compressor_add_buffer (z_compressor_t *self, const void *data, size_t size) {
  const uint8_t *pdata = (const uint8_t *)data;
  size_t n;

  if (self->bufavail != self->bufsize) {
    n = z_min(size, self->bufavail);
    z_memcpy(self->buffer + (self->bufsize - self->bufavail), pdata, n);
    self->bufavail -= n;
    pdata += n;
    size -= n;

    if (self->bufavail == 0) {
      if (!__compressor_flush_buffer(self, self->buffer, self->bufsize)) {
        return(1);
      }
    }
  }

  /* assert(self->bufavail == self->bufsize); */
  while (size >= self->bufsize) {
    n = z_align_down(size, self->bufsize);
    if (!__compressor_flush_buffer(self, pdata, n)) {
      return(2);
    }
    pdata += n;
    size -= n;
  }

  if (size > 0) {
    z_memcpy(self->buffer, pdata, size);
    self->bufavail -= size;
  }
  return(0);
}

/* ============================================================================
 *  PUBLIC Compressor methods
 */
void z_compressor_init (z_compressor_t *self,
                        void *buffer,
                        unsigned int bufsize,
                        void *dst_buffer,
                        unsigned int dst_size,
                        z_compress_t codec)
{
  self->dst_buffer = (uint8_t *)dst_buffer;
  self->dst_avail = dst_size;
  self->dst_size = dst_size;

  self->buffer = (uint8_t *)buffer;
  self->bufavail = bufsize;
  self->bufsize = bufsize;

  self->codec = codec;
  z_byte_slice_clear(&(self->bprev));
  self->iprev = 0;
}

int z_compressor_add (z_compressor_t *self, const void *data, unsigned int size) {
  if (__compressor_emergency_flush(self, size))
    return(1);
  return(__compressor_add_buffer(self, data, size));
}

int z_compressor_addv (z_compressor_t *self, const struct iovec *iov, int iovcnt) {
  const struct iovec *p;
  unsigned int size = 0;

  for (p = iov; iovcnt--; p++) {
    size += p->iov_len;
  }

  if (__compressor_emergency_flush(self, size))
    return(1);

  /* TODO: Keep the transaction */

  for (p = iov; iovcnt--; p++) {
    if (__compressor_add_buffer(self, p->iov_base, p->iov_len)) {
      return(1);
    }
  }
  return(0);
}

int z_compressor_add_bytes (z_compressor_t *self, const z_byte_slice_t *current)
{
  uint32_t prefix, suffix, length;
  struct iovec iov[2];
  uint8_t head[32];
  uint8_t *data;

  if (__compressor_emergency_flush(self, current->size))
    return(1);

  data = current->data;
  length = current->size;
  if (Z_UNLIKELY(z_byte_slice_is_empty(&(self->bprev)))) {
    prefix = 0;
    suffix = 0;
  } else {
    prefix = z_memshared(self->bprev.data, self->bprev.size, data, length);
    data += prefix;
    length -= prefix;
    suffix = z_memrshared(self->bprev.data + prefix, self->bprev.size - prefix, data, length);
    length -= suffix;
  }

  iov[0].iov_base = head;
  iov[0].iov_len  = z_encode_4int(head, prefix, suffix, length, 0);
  iov[1].iov_base = data;
  iov[1].iov_len  = length;
  return(z_compressor_addv(self, iov, 2));
}

int z_compressor_add_int (z_compressor_t *self, uint64_t current) {
  uint8_t buffer[16];
  int n;

  if (__compressor_emergency_flush(self, 4))
    return(1);

  Z_ASSERT(current >= self->iprev, "the assumption is that the numbers are sorted");
  n = z_encode_vint(buffer, current - self->iprev);
  self->iprev = current;
  return(__compressor_add_buffer(self, buffer, n));
}

int z_compressor_flush (z_compressor_t *self) {
  size_t size = self->bufsize - self->bufavail;
  if (size == 0)
    return(0);

  if (__compressor_flush_buffer(self, self->buffer, size)) {
    return(1);
  }
  return(0);
}

unsigned int z_compressor_flushed_size (const z_compressor_t *self) {
  return(self->dst_size - self->dst_avail);
}

unsigned int z_compressor_estimated_size (const z_compressor_t *self) {
  size_t bufused = (self->bufsize - self->bufavail);
  return(z_compressor_flushed_size(self) + bufused + __SAFETY_THRESHOLD(bufused));
}
