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

#ifndef _Z_IPC_H_
#define _Z_IPC_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/memslice.h>
#include <zcl/dbuffer.h>
#include <zcl/iopoll.h>
#include <zcl/stailq.h>
#include <zcl/opaque.h>

Z_TYPEDEF_STRUCT(z_ipc_msg_rdbuf)
Z_TYPEDEF_STRUCT(z_ipc_msg)

Z_TYPEDEF_STRUCT(z_ipc_msg_head)
Z_TYPEDEF_STRUCT(z_ipc_msg_client)
Z_TYPEDEF_STRUCT(z_ipc_msg_writer)
Z_TYPEDEF_STRUCT(z_ipc_msg_protocol)
Z_TYPEDEF_STRUCT(z_ipc_protocol)
Z_TYPEDEF_STRUCT(z_ipc_server)
Z_TYPEDEF_STRUCT(z_ipc_client)

#define Z_IPC_SERVER(x)              Z_CAST(z_ipc_server_t, x)
#define Z_IPC_CLIENT(x)              Z_CAST(z_ipc_client_t, x)
#define Z_IPC_MSG_CLIENT(x)          Z_CAST(z_ipc_msg_client_t, x)

#define __Z_IPC_CLIENT__             z_ipc_client_t __ipc_client__;
#define __Z_IPC_MSG_CLIENT__         z_ipc_msg_client_t __ipc_client__;

typedef int     (*z_ipc_msg_alloc_t)   (z_ipc_msg_client_t *client,
                                        z_ipc_msg_head_t *msg_head);
typedef int     (*z_ipc_msg_parse_t)   (z_ipc_msg_client_t *client,
                                        z_ipc_msg_head_t *msg_head);
typedef int     (*z_ipc_msg_exec_t)    (z_ipc_msg_client_t *client,
                                        z_ipc_msg_head_t *msg_head);
typedef int     (*z_ipc_msg_respond_t) (void *msg_ctx);
typedef void    (*z_ipc_msg_free_t)    (z_ipc_msg_client_t *client,
                                        void *msg_ctx);

struct z_ipc_protocol {
  /* raw-client protocol */
  int    (*read)          (z_ipc_client_t *client);
  int    (*write)         (z_ipc_client_t *client);
  /* client protocol */
  int    (*connected)     (z_ipc_client_t *client);
  int    (*disconnected)  (z_ipc_client_t *client);
  /* server protocol */
  int    (*uevent)        (z_ipc_server_t *server);
  int    (*timeout)       (z_ipc_server_t *server);
  int    (*accept)        (z_ipc_server_t *server);
  int    (*bind)          (const void *host, const void *service);
  void   (*unbind)        (z_ipc_server_t *server);
  int    (*setup)         (z_ipc_server_t *server);
};

struct z_ipc_msg_protocol {
  z_ipc_msg_alloc_t   msg_alloc;
  z_ipc_msg_parse_t   msg_parse;
  z_ipc_msg_exec_t    msg_exec;
  z_ipc_msg_respond_t msg_respond;
  z_ipc_msg_free_t    msg_free;
};

struct z_ipc_server {
  __Z_IOPOLL_ENTITY__
  const z_ipc_protocol_t *protocol;
  const z_ipc_msg_protocol_t *msg_protocol;
  const char *name;
  z_opaque_t  data;
  z_iopoll_entity_t timer;
  z_iopoll_entity_t event;
  uint16_t csize;
  uint16_t __pad;
  uint32_t connections;
};

struct z_ipc_client {
  __Z_IOPOLL_ENTITY__
};

struct z_ipc_msg_head {
  uint32_t msg_type;
  uint64_t msg_id;
  uint16_t body_length;
  uint32_t blob_length;
  uint8_t  pkt_type;
};

#define Z_IPC_MSG_IBUF      (64 + 32)
struct z_ipc_msg {
  z_slink_node_t node;
  uint32_t       hoffset;
  uint32_t       wtime;
  z_dbuf_node_t  dnode;
  uint8_t        data[Z_IPC_MSG_IBUF];
  uint8_t        rhead;
  uint8_t        hskip;
  uint32_t       latency;
};

struct z_ipc_msg_rdbuf {
  uint16_t parsed  : 1;
  uint16_t rd_data : 1;
  uint16_t hlength : 5;
  uint16_t rdcount : 9;
  uint16_t body_remaining;
  uint32_t blob_remaining;
  uint8_t *body_buffer;
  uint8_t *blob_buffer;
  uint8_t  head[20];
  uint32_t first_rd_usec;
};

struct z_ipc_msg_client {
  __Z_IPC_CLIENT__
  z_ipc_msg_rdbuf_t rdbuf;
  z_stailq_t        wmsgq;
  uint32_t          _pad;
  int32_t           refs;
};

#define z_ipc_client_set_writable(client, value)                               \
  z_iopoll_set_writable(Z_IOPOLL_ENTITY(client), value)

#define z_ipc_client_set_data_available(client, value)                         \
  z_iopoll_set_data_available(Z_IOPOLL_ENTITY(client), value)

#define z_ipc_plug(proto, msg_proto, name, client_type, addr, service)         \
  __z_ipc_plug(proto, msg_proto, name, sizeof(client_type), addr, service)

z_ipc_server_t *__z_ipc_plug             (const z_ipc_protocol_t *proto,
                                          const z_ipc_msg_protocol_t *msg_proto,
                                          const char *name,
                                          unsigned int csize,
                                          const void *address,
                                          const void *service);
void            z_ipc_unplug             (z_ipc_server_t *server);
int             z_ipc_server_add         (z_ipc_server_t *server, int csock);
z_ipc_server_t *z_ipc_get_server         (const z_ipc_client_t *client);

int             z_ipc_bind_tcp           (const void *hostname,
                                          const void *service);
int             z_ipc_accept_tcp         (z_ipc_server_t *server);

int             z_ipc_bind_udp           (const void *hostname,
                                          const void *service);
int             z_ipc_bind_udp_broadcast (const void *hostname,
                                          const void *service);

#ifdef Z_SOCKET_HAS_UNIX
int             z_ipc_bind_unix          (const void *path,
                                          const void *service);
void            z_ipc_unbind_unix        (z_ipc_server_t *server);
int             z_ipc_accept_unix        (z_ipc_server_t *server);
#endif /* Z_SOCKET_HAS_UNIX */

void            z_ipc_msg_set_length     (z_ipc_msg_t *self,
                                          uint8_t h_msg_len,
                                          uint32_t length);
z_ipc_msg_t *   z_ipc_msg_writer_open    (z_ipc_msg_t *self,
                                          const z_ipc_msg_head_t *head,
                                          z_dbuf_writer_t *writer,
                                          z_memory_t *memory);
void            z_ipc_msg_writer_close   (z_ipc_msg_t *self,
                                          z_dbuf_writer_t *writer,
                                          uint16_t body_length,
                                          uint32_t blob_length);
void            z_ipc_msg_reader_open    (z_ipc_msg_t *self,
                                          z_dbuf_reader_t *reader,
                                          z_memory_t *memory);

void            z_ipc_msg_client_open    (z_ipc_msg_client_t *self,
                                          const z_ipc_msg_protocol_t *proto);
void            z_ipc_msg_client_close   (z_ipc_msg_client_t *self,
                                          const z_ipc_msg_protocol_t *proto);
int             z_ipc_msg_client_read    (z_ipc_msg_client_t *self,
                                          const z_ipc_msg_protocol_t *proto);
void            z_ipc_msg_client_push    (z_ipc_msg_client_t *self,
                                          z_ipc_msg_t *msg);
int             z_ipc_msg_client_flush   (z_ipc_msg_client_t *self,
                                          const z_ipc_msg_protocol_t *proto);

__Z_END_DECLS__

#endif /* !_Z_IPC_H_ */
