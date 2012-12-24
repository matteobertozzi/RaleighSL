/*
 *   Copyright 2011-2012 Matteo Bertozzi
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
#include <zcl/memory.h>
#include <zcl/iopoll.h>

#define Z_IPC_PROTOCOL(x)        Z_CAST(z_ipc_protocol_t, x)
#define Z_IPC_SERVER(x)          Z_CAST(z_ipc_server_t, x)
#define Z_IPC_CLIENT(x)          Z_CAST(z_ipc_client_t, x)

#define __Z_IPC_CLIENT__         z_ipc_client_t __ipc_client__;

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

#define z_ipc_plug(memory, iopoll, proto, csize, addr, service, udata) \
    __z_ipc_plug(memory, iopoll, proto, sizeof(csize), addr, service, udata)

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

__Z_END_DECLS__

#endif /* !_Z_IPC_H_ */
