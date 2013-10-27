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

#ifndef _Z_RINGBUF_H_
#define _Z_RINGBUF_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/object.h>

Z_TYPEDEF_STRUCT(z_ringbuf)

struct z_ringbuf {
  uint8_t *buffer;
  size_t head;
  size_t tail;
  size_t size;
};

#define z_ringbuf_is_full(self)                   \
  ((self)->tail == ((self)->head ^ (self)->size))

#define z_ringbuf_is_empty(self)                  \
  ((self)->head == (self)->tail)

#define z_ringbuf_has_data(self)                  \
  (!z_ringbuf_is_empty(self))

#define z_ringbuf_avail(self)                     \
  ((self)->size - z_ringbuf_used(self))

int     z_ringbuf_alloc     (z_ringbuf_t *self, size_t size);
void    z_ringbuf_free      (z_ringbuf_t *self);

ssize_t z_ringbuf_fd_fetch  (z_ringbuf_t *self, int fd);
ssize_t z_ringbuf_fd_dump   (z_ringbuf_t *self, int fd);

size_t  z_ringbuf_rbuffer   (z_ringbuf_t *self, const uint8_t **buffer);
void    z_ringbuf_rback     (z_ringbuf_t *self, size_t n);
void    z_ringbuf_rskip     (z_ringbuf_t *self, size_t n);

void *  z_ringbuf_fetch     (z_ringbuf_t *self, void *buf, size_t n);
void    z_ringbuf_pop_iov   (z_ringbuf_t *self, struct iovec iov[2], size_t size);
size_t  z_ringbuf_pop       (z_ringbuf_t *self, void *buf, size_t n);

size_t  z_ringbuf_push      (z_ringbuf_t *self, const void *buf, size_t n);

size_t  z_ringbuf_used      (const z_ringbuf_t *self);

__Z_END_DECLS__

#endif /* _Z_RINGBUF_H_ */