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

#include <zcl/ipc-msg.h>
#include <zcl/macros.h>
#include <zcl/memory.h>
#include <zcl/iopoll.h>

#define Z_IPC_SERVER(entity)      z_container_of(entity, z_ipc_server_t, io_entity)
#define Z_IPC_CLIENT(entity)      z_container_of(entity, z_ipc_client_t, io_entity)
#define Z_IPC_MSG_CLIENT(entity)  z_container_of(entity, z_ipc_msg_client_t, ipc_client)

typedef struct z_ipc_protocol z_ipc_protocol_t;
typedef struct z_ipc_msg_client z_ipc_msg_client_t;
typedef struct z_ipc_client z_ipc_client_t;
typedef struct z_ipc_server z_ipc_server_t;

struct z_ipc_protocol {
  /* raw-client protocol */
  int    (*read)          (z_ipc_client_t *client);
  int    (*write)         (z_iopoll_engine_t *iopoll,
                           z_ipc_client_t *client);
  /* client protocol */
  int    (*connected)     (z_ipc_client_t *client);
  int    (*disconnected)  (z_ipc_client_t *client);
  /* server protocol */
  int    (*uevent)        (z_ipc_server_t *server);
  int    (*timeout)       (z_ipc_server_t *server);
  int    (*accept)        (z_ipc_server_t *server);
  int    (*bind)          (z_ipc_server_t *server,
                           const void *host,
                           const void *service);
  void   (*unbind)        (z_ipc_server_t *server);
  int    (*setup)         (z_ipc_server_t *server);
};

struct z_ipc_server {
  z_iopoll_entity_t io_entity;
  uint16_t csize;
  uint16_t __pad0;
  uint32_t connections;
  z_opaque_t udata;
/* 32 */
  const z_ipc_msg_protocol_t *msg_protocol;
  const z_ipc_protocol_t *protocol;
  z_iopoll_engine_t *engine;
  z_memory_t *memory;
/* 64 */
  z_iopoll_entity_t timer;
  z_iopoll_entity_t event;
  const char *name;
};

struct z_ipc_client {
  z_iopoll_entity_t io_entity;
};

struct z_ipc_msg_client {
  z_ipc_client_t ipc_client;
  z_ipc_msg_reader_t reader;
  z_ipc_msg_writer_t writer;
};

#define z_ipc_client_set_writable(engine, client, value)                       \
  z_iopoll_set_writable(engine, &((client)->io_entity), value)

#define z_ipc_client_set_data_available(client, value)                         \
  z_iopoll_set_data_available(&((client)->io_entity), value)

#define z_ipc_plug(server, engine, memory, proto, msg_proto, name, client_type, addr, service, udata)     \
  __z_ipc_plug(server, engine, memory, proto, msg_proto, name, sizeof(client_type), addr, service, udata)

#define z_ipc_get_server(type, client)   Z_CAST(type, __z_ipc_get_server(client))
#define z_ipc_get_server_memory(client)  (__z_ipc_get_server((client))->memory)

#define z_ipc_get_server_udata(type, client)                                \
  Z_OPAQUE_PTR(type, &(__z_ipc_get_server(Z_IPC_CLIENT(client))->udata))

z_ipc_server_t *__z_ipc_plug (z_ipc_server_t *server,
                              z_iopoll_engine_t *engine,
                              z_memory_t *memory,
                              const z_ipc_protocol_t *proto,
                              const z_ipc_msg_protocol_t *msg_proto,
                              const char *name,
                              unsigned int csize,
                              const void *address,
                              const void *service,
                              void *udata);
void z_ipc_unplug     (z_ipc_server_t *server);
int  z_ipc_server_add (z_ipc_server_t *server, int csock);

z_ipc_server_t *__z_ipc_get_server (const z_ipc_client_t *client);

int             z_ipc_bind_tcp           (z_ipc_server_t *server,
                                          const void *hostname,
                                          const void *service);
int             z_ipc_accept_tcp         (z_ipc_server_t *server);

int             z_ipc_bind_udp           (z_ipc_server_t *server,
                                          const void *hostname,
                                          const void *service);
int             z_ipc_bind_udp_broadcast (z_ipc_server_t *server,
                                          const void *hostname,
                                          const void *service);

#ifdef Z_SOCKET_HAS_UNIX
int             z_ipc_bind_unix          (z_ipc_server_t *server,
                                          const void *path,
                                          const void *service);
void            z_ipc_unbind_unix        (z_ipc_server_t *server);
int             z_ipc_accept_unix        (z_ipc_server_t *server);
#endif /* Z_SOCKET_HAS_UNIX */

__Z_END_DECLS__

#endif /* !_Z_IPC_H_ */
