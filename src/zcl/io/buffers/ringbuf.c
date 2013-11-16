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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include <zcl/debug.h>
#include <zcl/ringbuf.h>
#include <zcl/global.h>
#include <zcl/string.h>

/* ===========================================================================
 *  PRIVATE Ringbuf methods
 */
static int __ringbuf_write_iov (z_ringbuf_t *self, struct iovec iov[2]) {
  if (z_ringbuf_is_full(self)) {
    iov[0].iov_len = 0;
    iov[1].iov_len = 0;
    return(0);
  }

  size_t mask = self->size - 1;
  size_t head = self->head & mask;
  size_t tail = self->tail & mask;
  if (head <= tail) {
    iov[0].iov_base = self->buffer + tail;
    iov[0].iov_len  = self->size - tail;
    iov[1].iov_base = self->buffer;
    iov[1].iov_len  = head;
    return(2);
  }

  iov[0].iov_base = self->buffer + tail;
  iov[0].iov_len = head - tail;
  iov[1].iov_len = 0;
  return(1);
}

static size_t __ringbuf_read_iov (z_ringbuf_t *self, struct iovec iov[2]) {
  if (z_ringbuf_is_empty(self)) {
    iov[0].iov_base = NULL;
    iov[0].iov_len = 0;
    iov[1].iov_base = NULL;
    iov[1].iov_len = 0;
    return(0);
  }

  size_t mask = self->size - 1;
  size_t head = self->head & mask;
  size_t tail = self->tail & mask;
  if (head <= tail) {
    iov[0].iov_base = self->buffer + head;
    iov[0].iov_len = tail - head;
    iov[1].iov_len = 0;
    return(iov[0].iov_len);
  }

  iov[0].iov_base = self->buffer + head;
  iov[0].iov_len  = self->size - head;
  iov[1].iov_base = self->buffer;
  iov[1].iov_len  = tail;
  return(iov[0].iov_len + iov[1].iov_len);
}

/* ===========================================================================
 *  PUBLIC Ringbuf methods
 */
int z_ringbuf_alloc (z_ringbuf_t *self, size_t size) {
  size = z_align_up(size, 8);
  self->buffer = z_memory_alloc(z_global_memory(), uint8_t, size);
  if (Z_MALLOC_IS_NULL(self->buffer))
    return(1);

  self->size = size;
  self->head = 0;
  self->tail = 0;
  return(0);
}

void z_ringbuf_free (z_ringbuf_t *self) {
  z_memory_free(z_global_memory(), self->buffer);
}

ssize_t z_ringbuf_fd_fetch (z_ringbuf_t *self, int fd) {
  size_t avail;
  ssize_t rd;

  if (z_ringbuf_is_full(self))
    return(0);

  size_t mask = self->size - 1;
  size_t head = self->head & mask;
  size_t tail = self->tail & mask;
  if (head <= tail) {
    /*
     * -- iov[1] ---               -- iov[0] ---
     * +---------------------------------------+
     * |   |   |   | A | B | C | D |   |   |   |
     * +---------------------------------------+
     *            ^head            ^tail
     */
    struct iovec iov[2];
    iov[0].iov_base = self->buffer + tail;
    iov[0].iov_len  = self->size - tail;
    iov[1].iov_base = self->buffer;
    iov[1].iov_len  = head;
    avail = iov[0].iov_len + iov[1].iov_len;
    rd = readv(fd, iov, 2);
  } else {
    /*
     *     -- iov[0] ---
     * +-----------------------------------+
     * | G |   |   |   | B | C | D | E | F |
     * +-----------------------------------+
     *     ^tail       ^head
     */
    avail = head - tail;
    rd = read(fd, self->buffer + tail, avail);
  }

  if (rd > 0) {
    mask = (self->size << 1) - 1;
    if (rd == avail)
      self->head = self->head & mask;
    self->tail = (self->tail + rd) & mask;
  }

  return(rd);
}

ssize_t z_ringbuf_fd_dump (z_ringbuf_t *self, int fd) {
  ssize_t wr;

  if (z_ringbuf_is_empty(self))
    return(0);

  size_t mask = self->size - 1;
  size_t head = self->head & mask;
  size_t tail = self->tail & mask;
  if (head <= tail) {
    /*
     *              ---- iov[0] ----
     * +---------------------------------------+
     * |   |   |   | A | B | C | D |   |   |   |
     * +---------------------------------------+
     *            ^head            ^tail
     */
    wr = write(fd, self->buffer + head, tail - head);
  } else {
    /*
     * -- iov[1] ---           -- iov[0] ---
     * +-----------------------------------+
     * | G | H | I |   |   |   | D | E | F |
     * +-----------------------------------+
     *             ^tail       ^head
     */
    struct iovec iov[2];
    iov[0].iov_base = self->buffer + head;
    iov[0].iov_len  = self->size - head;
    iov[1].iov_base = self->buffer;
    iov[1].iov_len  = tail;
    wr = writev(fd, iov, 2);
  }

  if (wr > 0) {
    self->head = (self->head + wr) & ((self->size << 1) - 1);
  }
  return(wr);
}

size_t z_ringbuf_rbuffer (z_ringbuf_t *self, const uint8_t **buffer) {
  size_t n;

  if (z_ringbuf_is_empty(self))
    return(0);

  size_t mask = self->size - 1;
  size_t head = self->head & mask;
  size_t tail = self->tail & mask;

  *buffer = self->buffer + head;
  n = (head < tail) ? (tail - head) : (self->size - head);

  self->head = (self->head + n) & ((self->size << 1) - 1);
  return(n);
}

void z_ringbuf_rback (z_ringbuf_t *self, size_t n) {
  self->head = (self->head - n) & ((self->size << 1) - 1);
}

void z_ringbuf_rskip (z_ringbuf_t *self, size_t n) {
  self->head = (self->head + n) & ((self->size << 1) - 1);
}

void *z_ringbuf_fetch (z_ringbuf_t *self, void *buf, size_t n) {
  struct iovec iov[2];
  size_t avail;

  avail = __ringbuf_read_iov(self, iov);
  if (n <= iov[0].iov_len)
    return(iov[0].iov_base);

  if (n <= avail) {
    uint8_t *pbuf = (uint8_t *)buf;

    z_memcpy(pbuf, iov[0].iov_base, iov[0].iov_len);
    pbuf += iov[0].iov_len;
    n -= iov[0].iov_len;

    /* assert(n <= iov[1].iov_len); */
    z_memcpy(pbuf, iov[1].iov_base, n);
    return(buf);
  }

  return(NULL);
}

void z_ringbuf_pop_iov (z_ringbuf_t *self, struct iovec iov[2], size_t size) {
  __ringbuf_read_iov(self, iov);
  if (size <= iov[0].iov_len) {
    iov[0].iov_len = z_min(size, iov[0].iov_len);
    iov[1].iov_len = 0;
  } else {
    size -= iov[0].iov_len;
    iov[1].iov_len = z_min(size, iov[1].iov_len);
  }
}

size_t z_ringbuf_pop (z_ringbuf_t *self, void *buf, size_t n) {
  uint8_t *pbuf = (uint8_t *)buf;
  struct iovec iov[2];
  size_t total;

  __ringbuf_read_iov(self, iov);
  if (n > iov[0].iov_len) {
    z_memcpy(pbuf, iov[0].iov_base, iov[0].iov_len);
    pbuf += iov[0].iov_len;
    n -= iov[0].iov_len;
    n = z_min(n, iov[1].iov_len);
    z_memcpy(pbuf, iov[1].iov_base, n);
    total = n + iov[0].iov_len;
  } else {
    total = z_min(n, iov[0].iov_len);
    z_memcpy(pbuf, iov[0].iov_base, total);
  }

  self->head = (self->head + total) & ((self->size << 1) - 1);
  return(total);
}

size_t z_ringbuf_push (z_ringbuf_t *self, const void *buf, size_t n) {
  const uint8_t *pbuf = (const uint8_t *)buf;
  struct iovec iov[2];
  size_t total;
  size_t mask;

  __ringbuf_write_iov(self, iov);
  if (n > iov[0].iov_len) {
    z_memcpy(iov[0].iov_base, pbuf, iov[0].iov_len);
    pbuf += iov[0].iov_len;
    n -= iov[0].iov_len;
    n = z_min(n, iov[1].iov_len);
    z_memcpy(iov[1].iov_base, pbuf, n);
    total = n + iov[0].iov_len;
  } else {
    total = z_min(n, iov[0].iov_len);
    z_memcpy(iov[0].iov_base, pbuf, total);
  }

  mask = (self->size << 1) - 1;
  if (total == (iov[0].iov_len + iov[1].iov_len))
    self->head = self->head & mask;
  self->tail = (self->tail + total) & mask;
  return(total);
}

size_t z_ringbuf_used (const z_ringbuf_t *self) {
  if (z_ringbuf_is_empty(self))
    return(0);

  size_t mask = self->size - 1;
  size_t head = self->head & mask;
  size_t tail = self->tail & mask;
  return((head <= tail) ? (tail - head) : ((self->size - head) + tail));
}
