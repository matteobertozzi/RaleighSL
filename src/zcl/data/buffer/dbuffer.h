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

#ifndef _Z_DBUFFER_H_
#define _Z_DBUFFER_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/memory.h>
#include <zcl/memref.h>

Z_TYPEDEF_STRUCT(z_dbuf_writer)
Z_TYPEDEF_STRUCT(z_dbuf_reader)
Z_TYPEDEF_STRUCT(z_dbuf_node)

#define Z_DBUF_NO_HEAD          (0xff)
#define Z_DBUF_EOF              (0xff)
#define Z_DBUF_REF              (0xfe)
#define Z_DBUF_IBUFLEN          (1)

struct z_dbuf_node {
  z_dbuf_node_t *next;
  uint8_t size  : 7;
  uint8_t alloc : 1;
  uint8_t data[1];
} __attribute__((packed));

struct z_dbuf_writer {
  z_dbuf_node_t *head;
  z_dbuf_node_t *tail;

  uint8_t *   pbuf;
  uint8_t     phead;
  uint8_t     avail;
  uint16_t    _pad;
  uint32_t    size;
  z_memory_t *memory;
};

struct z_dbuf_reader {
  z_dbuf_node_t *head;
  uint32_t       phead;
  uint32_t       poffset;
  z_memory_t *   memory;
};

#define z_dbuf_node_size(node)      (((node)->size << 1) + 1)

#define z_dbuf_node_set_size(node, size_)                   \
  do {                                                      \
    (node)->size = (z_align_down(size_, 2) >> 1);           \
  } while (0)

void      z_dbuffer_clear       (z_memory_t *memory,
                                 z_dbuf_node_t *head);


int       z_dbuf_writer_init    (z_dbuf_writer_t *self,
                                 z_memory_t *memory,
                                 z_dbuf_node_t *tail,
                                 uint8_t phead,
                                 uint8_t avail);

uint8_t * z_dbuf_writer_next    (z_dbuf_writer_t *self,
                                 uint8_t *buffer,
                                 uint8_t size);
void      z_dbuf_writer_move    (z_dbuf_writer_t *self,
                                 uint8_t size);
uint8_t * z_dbuf_writer_commit  (z_dbuf_writer_t *self,
                                 uint8_t *buffer,
                                 uint8_t size);

int       z_dbuf_writer_add     (z_dbuf_writer_t *self,
                                 const void *data,
                                 size_t size);
int       z_dbuf_writer_add_ref (z_dbuf_writer_t *self,
                                 const z_memref_t *ref);


void      z_dbuf_reader_init    (z_dbuf_reader_t *self,
                                 z_memory_t *memory,
                                 z_dbuf_node_t *head,
                                 uint8_t phead,
                                 int poffset);
int       z_dbuf_reader_iovs    (z_dbuf_reader_t *self,
                                 struct iovec *iovs,
                                 int iovcnt);
size_t    z_dbuf_reader_remove  (z_dbuf_reader_t *self,
                                 size_t rm_size);

__Z_END_DECLS__

#endif /* !_Z_DBUFFER_H_ */
