/*
 *   Copyright 2011-2013 Matteo Bertozzi
 *
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

#include <zcl/macros.h>
#include <zcl/object.h>
#include <zcl/reader.h>
#include <zcl/iopoll.h>

#define Z_IPC_PROTOCOL(x)        Z_CAST(z_ipc_protocol_t, x)
#define Z_IPC_SERVER(x)          Z_CAST(z_ipc_server_t, x)
#define Z_IPC_CLIENT(x)          Z_CAST(z_ipc_client_t, x)

#define Z_CONST_IPC_MSG(x)       Z_CONST_CAST(z_ipc_msg_t, x)
#define Z_IPC_MSG_READER(x)      Z_CAST(z_ipc_msg_reader_t, x)
#define Z_IPC_MSG(x)             Z_CAST(z_ipc_msg_t, x)

#define __Z_IPC_CLIENT__         z_ipc_client_t __ipc_client__;

Z_TYPEDEF_STRUCT(z_ipc_msg_interfaces)
Z_TYPEDEF_STRUCT(z_ipc_msg_reader)
Z_TYPEDEF_STRUCT(z_ipc_msg)

Z_TYPEDEF_STRUCT(z_ipc_msgbuf_node)
Z_TYPEDEF_STRUCT(z_ipc_msgbuf)

Z_TYPEDEF_STRUCT(z_ipc_protocol)
Z_TYPEDEF_STRUCT(z_ipc_server)
Z_TYPEDEF_STRUCT(z_ipc_client)

struct z_ipc_protocol {
    /* server protocol */
    int    (*bind)          (const void *host, const void *service);
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
    z_memory_t *memory;
    z_iopoll_t *iopoll;
    void *udata;
    unsigned int csize;
};

struct z_ipc_client {
    __Z_IOPOLL_ENTITY__
    const z_ipc_server_t *server;
};

struct z_ipc_msgbuf {
    z_memory_t *memory;
    z_ipc_msgbuf_node_t *head;
    z_ipc_msgbuf_node_t *tail;
    uint64_t length;
    uint32_t offset;
    int refs;
};

struct z_ipc_msg_interfaces {
    Z_IMPLEMENT_TYPE
    Z_IMPLEMENT_READER
};

struct z_ipc_msg_reader {
    __Z_READABLE__
    z_ipc_msgbuf_node_t *node;
    uint64_t available;
    uint32_t offset;
};

struct z_ipc_msg {
    __Z_OBJECT__(z_ipc_msg)
    z_ipc_msgbuf_t *msgbuf;
    z_ipc_msgbuf_node_t *head;
    unsigned int blocks;
    unsigned int offset;
    uint64_t length;
    uint16_t id;
};

#define z_ipc_plug(memory, iopoll, proto, csize, addr, service, udata)       \
    __z_ipc_plug(memory, iopoll, proto, sizeof(csize), addr, service, udata)

#define z_ipc_client_server(client)    Z_IPC_CLIENT(client)->server
#define z_ipc_client_iopoll(client)    z_ipc_client_server(client)->iopoll
#define z_ipc_client_memory(client)    z_ipc_client_server(client)->memory

#define z_ipc_client_set_writable(client, value)                             \
    z_iopoll_set_writable(z_ipc_client_iopoll(client),                       \
        Z_IOPOLL_ENTITY(client), value);

int             __z_ipc_plug        (z_memory_t *memory,
                                     z_iopoll_t *iopoll,
                                     const z_ipc_protocol_t *protocol,
                                     unsigned int csize,
                                     const void *address,
                                     const void *service,
                                     void *user_data);

int             z_ipc_bind_tcp      (const void *hostname,
                                     const void *service);
int             z_ipc_accept_tcp    (z_ipc_server_t *server);

#ifdef Z_SOCKET_HAS_UNIX
int             z_ipc_bind_unix     (const void *path,
                                     const void *service);
int             z_ipc_accept_unix   (z_ipc_server_t *server);
#endif /* Z_SOCKET_HAS_UNIX */

z_ipc_msg_t *   z_ipc_msg_alloc         (z_ipc_msg_t *self,
                                         z_memory_t *memory);
void            z_ipc_msg_free          (z_ipc_msg_t *self);

void            z_ipc_msgbuf_open       (z_ipc_msgbuf_t *msgbuf,
                                         z_memory_t *memory);
void            z_ipc_msgbuf_close      (z_ipc_msgbuf_t *msgbuf);
void            z_ipc_msgbuf_clear      (z_ipc_msgbuf_t *msgbuf);
int             z_ipc_msgbuf_add        (z_ipc_msgbuf_t *msgbuf, int fd);
z_ipc_msg_t*    z_ipc_msgbuf_get        (z_ipc_msgbuf_t *msgbuf,
                                         z_ipc_msg_t *msg);
void            z_ipc_msgbuf_release    (z_ipc_msgbuf_t *msgbuf,
                                         z_ipc_msg_t *msg);

__Z_END_DECLS__

#endif /* !_Z_IPC_H_ */
