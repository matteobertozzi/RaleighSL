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
#include <zcl/ref.h>

typedef struct z_dbuf_reader z_dbuf_reader_t;
typedef struct z_dbuf_writer z_dbuf_writer_t;
typedef struct z_dbuf_iovec z_dbuf_iovec_t;
typedef struct z_dbuf_vref z_dbuf_vref_t;
typedef struct z_dbuf_node z_dbuf_node_t;

#define Z_DBUF_NIOVS      (8)

struct z_dbuf_node {
  z_dbuf_node_t *next;
  uint8_t length;
  uint8_t data[1];
} __attribute__((packed));

struct z_dbuf_iovec {
  uint8_t count;
  uint8_t offset;
  uint8_t index;
  uint8_t __pad;
  uint32_t consumed;
  z_dbuf_node_t *node;

  struct iovec   iov[Z_DBUF_NIOVS];
  uint8_t        off[Z_DBUF_NIOVS];
  uint8_t        idx[Z_DBUF_NIOVS];
  z_dbuf_node_t *nod[Z_DBUF_NIOVS];
  z_dbuf_vref_t *vrf[Z_DBUF_NIOVS];
  z_ref_data_t * ref[Z_DBUF_NIOVS];
};

struct z_dbuf_reader {
  uint8_t  offset;
  uint8_t  index;
  uint8_t  __pad[2];
  uint32_t consumed;
  z_dbuf_node_t *head;
  z_memory_t *memory;
};

struct z_dbuf_writer {
  uint8_t  type;
  uint8_t  avail;
  uint8_t  size_offset;
  uint8_t  data_offset;
  uint32_t total;

  z_dbuf_node_t *tail;
  z_dbuf_node_t *head;
  z_memory_t *memory;
};

void            z_dbuf_writer_init        (z_dbuf_writer_t *self,
                                           z_memory_t *memory);
int             z_dbuf_writer_add_node    (z_dbuf_writer_t *self,
                                           unsigned int size);

int             z_dbuf_writer_add         (z_dbuf_writer_t *self,
                                           const void *buffer,
                                           size_t size);
uint8_t *       z_dbuf_writer_next        (z_dbuf_writer_t *self,
                                           uint32_t *avail);
uint8_t *       z_dbuf_writer_get         (z_dbuf_writer_t *self,
                                           uint8_t *buffer,
                                           uint32_t size);
int             z_dbuf_writer_commit      (z_dbuf_writer_t *self,
                                           const void *buffer,
                                           uint32_t size);

uint8_t *       z_dbuf_writer_mark        (z_dbuf_writer_t *self,
                                           const uint8_t size);

int             z_dbuf_writer_add_ref     (z_dbuf_writer_t *self,
                                           const z_ref_vtable_t *ref_vtable,
                                           uint8_t *data,
                                           uint32_t offset,
                                           uint32_t length,
                                           void *udata);
z_ref_data_t *  z_dbuf_writer_next_refs   (z_dbuf_writer_t *self,
                                           int *avail_refs);
void            z_dbuf_writer_commit_refs (z_dbuf_writer_t *self,
                                           const z_ref_vtable_t *ref_vtable,
                                           void *udata,
                                           const z_ref_data_t *refs,
                                           int count);

void            z_dbuf_reader_init      (z_dbuf_reader_t *self,
                                         z_memory_t *memory,
                                         z_dbuf_node_t *head,
                                         uint8_t offset,
                                         uint8_t index,
                                         uint32_t consumed);
void            z_dbuf_reader_get       (z_dbuf_reader_t *self,
                                         z_dbuf_iovec_t *iovec);
void            z_dbuf_reader_remove    (z_dbuf_reader_t *self,
                                         z_dbuf_iovec_t *iovec,
                                         ssize_t rdsize);
void            z_dbuf_reader_clear     (z_dbuf_reader_t *self);

__Z_END_DECLS__

#endif /* !_Z_DBUFFER_H_ */
