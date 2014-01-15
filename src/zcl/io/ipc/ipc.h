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

#include <zcl/ringbuf.h>
#include <zcl/locking.h>
#include <zcl/reader.h>
#include <zcl/macros.h>
#include <zcl/object.h>
#include <zcl/iopoll.h>

#define Z_IPC_PROTOCOL(x)        Z_CAST(z_ipc_protocol_t, x)
#define Z_IPC_SERVER(x)          Z_CAST(z_ipc_server_t, x)
#define Z_IPC_CLIENT(x)          Z_CAST(z_ipc_client_t, x)

#define __Z_IPC_CLIENT__         z_ipc_client_t __ipc_client__;

Z_TYPEDEF_STRUCT(z_ipc_msg_protocol)
Z_TYPEDEF_STRUCT(z_ipc_protocol)
Z_TYPEDEF_STRUCT(z_ipc_server)
Z_TYPEDEF_STRUCT(z_ipc_client)
Z_TYPEDEF_STRUCT(z_ipc_msgbuf)

typedef int (*z_ipc_msg_parse_t)     (z_iopoll_entity_t *client,
                                      const struct iovec iov[2]);

struct z_ipc_protocol {
  /* server protocol */
  int    (*bind)          (const void *host, const void *service);
  void   (*unbind)        (z_ipc_server_t *server);
  int    (*accept)        (z_ipc_server_t *server);
  int    (*setup)         (z_ipc_server_t *server);
  /* client protocol */
  int    (*connected)     (z_ipc_client_t *client);
  void   (*disconnected)  (z_ipc_client_t *client);
  int    (*read)          (z_ipc_client_t *client);
  int    (*write)         (z_ipc_client_t *client);
};

struct z_ipc_server {
  __Z_IOPOLL_ENTITY__
  const z_ipc_protocol_t *protocol;
  z_iopoll_t *iopoll;
  void *udata;
  unsigned int csize;
};

struct z_ipc_client {
  __Z_IOPOLL_ENTITY__
  const z_ipc_server_t *server;
};

struct z_ipc_msgbuf {
  z_ringbuf_t ibuffer;

  struct obuffer {
    void *tail;
    void *head;
    z_spinlock_t lock;
    unsigned int m_count;
    unsigned int m_offset;
    unsigned int d_offset;
  } obuffer;

  uint8_t     version;
};

#define z_ipc_plug(iopoll, proto, csize, addr, service, udata)       \
  __z_ipc_plug(iopoll, proto, sizeof(csize), addr, service, udata)

#define z_ipc_client_server(client)    Z_IPC_CLIENT(client)->server
#define z_ipc_client_iopoll(client)    z_ipc_client_server(client)->iopoll

#define z_ipc_client_set_writable(client, value)                             \
  z_iopoll_set_writable(z_ipc_client_iopoll(client),                         \
                        Z_IOPOLL_ENTITY(client), value);

z_ipc_server_t *__z_ipc_plug        (z_iopoll_t *iopoll,
                                     const z_ipc_protocol_t *protocol,
                                     unsigned int csize,
                                     const void *address,
                                     const void *service,
                                     void *user_data);
void            z_ipc_unplug        (z_iopoll_t *iopoll,
                                     z_ipc_server_t *server);

int             z_ipc_bind_tcp      (const void *hostname,
                                     const void *service);
int             z_ipc_accept_tcp    (z_ipc_server_t *server);

#ifdef Z_SOCKET_HAS_UNIX
int             z_ipc_bind_unix     (const void *path,
                                     const void *service);
void            z_ipc_unbind_unix   (z_ipc_server_t *server);
int             z_ipc_accept_unix   (z_ipc_server_t *server);
#endif /* Z_SOCKET_HAS_UNIX */


int             z_ipc_msgbuf_open   (z_ipc_msgbuf_t *msgbuf,
                                     size_t isize);
void            z_ipc_msgbuf_close  (z_ipc_msgbuf_t *msgbuf);
int             z_ipc_msgbuf_fetch  (z_ipc_msgbuf_t *msgbuf,
                                     z_iopoll_entity_t *client,
                                     z_ipc_msg_parse_t msg_parse_func);
int             z_ipc_msgbuf_push   (z_ipc_msgbuf_t *self,
                                     void *buf,
                                     size_t n);
int             z_ipc_msgbuf_flush  (z_ipc_msgbuf_t *msgbuf,
                                     z_iopoll_t *iopoll,
                                     z_iopoll_entity_t *client);

__Z_END_DECLS__

#endif /* _Z_IPC_H_ */
