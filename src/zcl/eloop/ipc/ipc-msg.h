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

#ifndef _Z_IPC_MSG_H_
#define _Z_IPC_MSG_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/histogram.h>
#include <zcl/dbuffer.h>
#include <zcl/macros.h>
#include <zcl/dlink.h>
#include <zcl/io.h>

typedef struct z_ipc_msg_protocol z_ipc_msg_protocol_t;
typedef struct z_ipc_msg_writer z_ipc_msg_writer_t;
typedef struct z_ipc_msg_reader z_ipc_msg_reader_t;
typedef struct z_ipc_msg_stats z_ipc_msg_stats_t;
typedef struct z_ipc_msg_head z_ipc_msg_head_t;
typedef struct z_ipc_msg_buf z_ipc_msg_buf_t;

struct z_ipc_msg_stats {
  z_histogram_t latency;
  uint64_t      latency_events[22];

  z_histogram_t fwd_size;
  uint64_t      fwd_size_events[26];

  z_histogram_t body_size;
  uint64_t      body_size_events[26];

  z_histogram_t data_size;
  uint64_t      data_size_events[26];
};

struct z_ipc_msg_head {
  uint32_t msg_type;
  uint32_t fwd_length;
  uint32_t body_length;
  uint32_t data_length;
  uint64_t msg_id;
};

struct z_ipc_msg_reader {
  uint8_t  state;
  uint8_t  rdmore;
  uint16_t read_calls;
  uint32_t buflen;
  uint64_t first_read_ts;
  union {
    uint8_t hbuf[24];
    z_ipc_msg_head_t hmsg;
  } h;
  uint8_t *fwd;
  uint8_t *body;
  uint8_t *data;
};

struct z_ipc_msg_buf {
  z_dlink_node_t node;
  /* dbuffer */
  z_dbuf_node_t *head;
  z_dbuf_node_t *tail;
  uint32_t bufsize;
  uint32_t consumed;
  uint8_t  offset;
  uint8_t  index;
  uint8_t  __pad[2];
  /* stats */
  uint32_t write_calls;
  uint64_t req_ts;
  uint64_t resp_ts;
};

struct z_ipc_msg_writer {
  z_dlink_node_t head;
};

struct z_ipc_msg_protocol {
  int  (*alloc)   (z_ipc_msg_reader_t *self,
                   uint8_t pkg_type,
                   const z_ipc_msg_head_t *msg_head,
                   void *udata);
  int  (*publish) (z_ipc_msg_reader_t *self,
                   const z_ipc_msg_head_t *msg_head,
                   void *udata);
  void (*remove)  (z_ipc_msg_writer_t *self,
                   z_ipc_msg_buf_t *msg,
                   void *udata);
};

void z_ipc_msg_stats_init        (z_ipc_msg_stats_t *self);
void z_ipc_msg_stats_dump        (z_ipc_msg_stats_t *self,
                                  const char *msg_type,
                                  FILE *stream);
void z_ipc_msg_stats_add         (z_ipc_msg_stats_t *self,
                                  const z_ipc_msg_head_t *msg_head);
void z_ipc_msg_stats_add_latency (z_ipc_msg_stats_t *self,
                                  uint64_t latency);

void z_ipc_msg_reader_open  (z_ipc_msg_reader_t *self);
void z_ipc_msg_reader_close (z_ipc_msg_reader_t *self);
int  z_ipc_msg_reader_read  (z_ipc_msg_reader_t *self,
                             const z_io_seq_vtable_t *io_proto,
                             const z_ipc_msg_protocol_t *proto,
                             void *udata);

z_ipc_msg_buf_t *z_ipc_msg_buf_alloc        (z_memory_t *memory);
void             z_ipc_msg_buf_free         (z_ipc_msg_buf_t *msg,
                                             z_memory_t *memory);
void             z_ipc_msg_buf_init         (z_ipc_msg_buf_t *self);
void             z_ipc_msg_buf_writer_open  (z_ipc_msg_buf_t *self,
                                             z_dbuf_writer_t *writer,
                                             z_memory_t *memory);
void             z_ipc_msg_buf_writer_close (z_ipc_msg_buf_t *self,
                                             z_dbuf_writer_t *writer);
int              z_ipc_msg_buf_write_head   (z_ipc_msg_buf_t *self,
                                             z_dbuf_writer_t *writer,
                                             uint8_t pkg_type,
                                             const z_ipc_msg_head_t *head);
void             z_ipc_msg_buf_edit_head    (z_ipc_msg_buf_t *self,
                                             z_dbuf_writer_t *writer,
                                             uint32_t body_length,
                                             uint32_t data_length);

void    z_ipc_msg_writer_open            (z_ipc_msg_writer_t *self);
void    z_ipc_msg_writer_close           (z_ipc_msg_writer_t *self);
void    z_ipc_msg_writer_clear           (z_ipc_msg_writer_t *self,
                                          z_memory_t *memory);
void    z_ipc_msg_writer_push            (z_ipc_msg_writer_t *self,
                                          z_ipc_msg_buf_t *msg);
int     z_ipc_msg_writer_flush           (z_ipc_msg_writer_t *self,
                                          z_memory_t *memory,
                                          const z_io_seq_vtable_t *io_proto,
                                          const z_ipc_msg_protocol_t *proto,
                                          void *udata);
#define z_ipc_msg_writer_has_data(self)  z_dlink_is_not_empty(&((self)->head))

__Z_END_DECLS__

#endif /* !_Z_IPC_MSG_H_ */
