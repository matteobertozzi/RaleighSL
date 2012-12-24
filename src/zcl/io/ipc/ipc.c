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
#include <unistd.h>

#include <zcl/ipc.h>
#include <zcl/fd.h>

static const z_vtable_iopoll_entity_t __ipc_server_vtable;
static const z_vtable_iopoll_entity_t __ipc_client_vtable;

/* ============================================================================
 *  RPC client iopoll entity
 */
static z_ipc_client_t *__ipc_client_alloc (z_ipc_server_t *server, int csock) {
    z_memory_t *memory = server->memory;
    z_ipc_client_t *client;

    /* Allocate client */
    if (!(client = z_memory_alloc(memory, z_ipc_client_t, server->csize)))
        return(NULL);

    /* Initialize client */
    z_iopoll_entity_init(Z_IOPOLL_ENTITY(client), &__ipc_client_vtable, csock);
    client->server = server;

    /* Ask the protocol to do its own stuff before starting up */
    if (server->protocol->connected != NULL) {
        if (server->protocol->connected(client)) {
            z_memory_free(server->memory, client);
            return(NULL);
        }
    }

    return(client);
}

static void __ipc_client_free (z_iopoll_t *iopoll, z_ipc_client_t *client) {
    const z_ipc_server_t *server = client->server;

    /* Ask the protocol to do its own stuff before closing down */
    if (server->protocol->disconnected != NULL)
        server->protocol->disconnected(client);

    /* Close client description */
    close(Z_IOPOLL_ENTITY_FD(client));

    /* Deallocate client */
    z_memory_free(server->memory, client);
}

static void __ipc_client_close (z_iopoll_t *iopoll, z_iopoll_entity_t *client) {
    __ipc_client_free(iopoll, Z_IPC_CLIENT(client));
}

static int __ipc_client_read (z_iopoll_t *iopoll, z_iopoll_entity_t *entity) {
    const z_ipc_protocol_t *proto = Z_IPC_CLIENT(entity)->server->protocol;
    return((proto->read != NULL) ? proto->read(Z_IPC_CLIENT(entity)) : 0);
}

static int __ipc_client_write (z_iopoll_t *iopoll, z_iopoll_entity_t *entity) {
    const z_ipc_protocol_t *proto = Z_IPC_CLIENT(entity)->server->protocol;
    return((proto->write != NULL) ? proto->write(Z_IPC_CLIENT(entity)) : 0);
}

/* ============================================================================
 *  IPC Server Private Methods
 */
static z_ipc_server_t *__ipc_server_alloc (z_memory_t *memory,
                                           const z_ipc_protocol_t *proto,
                                           const void *hostname,
                                           const void *service)
{
    z_ipc_server_t *server;
    int socket;

    if ((server = z_memory_struct_alloc(memory, z_ipc_server_t)) == NULL)
        return(NULL);

    if ((socket = proto->bind(hostname, service)) < 0) {
        z_memory_struct_free(memory, z_ipc_server_t, server);
        return(NULL);
    }

    if (z_fd_set_blocking(socket, 0)) {
        close(socket);
        z_memory_struct_free(memory, z_ipc_server_t, server);
        return(NULL);
    }

    z_iopoll_entity_init(Z_IOPOLL_ENTITY(server), &__ipc_server_vtable, socket);
    server->protocol = proto;
    server->memory = memory;

    if (proto->setup != NULL && proto->setup(server)) {
        close(socket);
        z_memory_struct_free(memory, z_ipc_server_t, server);
        return(NULL);
    }

    return(server);
}

static void __ipc_server_free (z_iopoll_t *iopoll, z_ipc_server_t *server) {
    close(Z_IOPOLL_ENTITY_FD(server));
    z_memory_struct_free(server->memory, z_ipc_server_t, server);
}

static void __ipc_server_close (z_iopoll_t *iopoll, z_iopoll_entity_t *server) {
    __ipc_server_free(iopoll, Z_IPC_SERVER(server));
}

static int __ipc_server_accept (z_iopoll_t *iopoll, z_iopoll_entity_t *entity) {
    z_ipc_server_t *server = Z_IPC_SERVER(entity);
    z_ipc_client_t *client;
    int csock;

    if ((csock = server->protocol->accept(server)) < 0)
        return(-1);

    if ((client = __ipc_client_alloc(server, csock)) == NULL) {
        close(csock);
        return(-2);
    }

    if (z_iopoll_add(server->iopoll, Z_IOPOLL_ENTITY(client))) {
        Z_IOPOLL_ENTITY(client)->vtable->close(iopoll, Z_IOPOLL_ENTITY(client));
        return(-4);
    }

    return(0);
}

/* ============================================================================
 *  IPC vtables
 */
static const z_vtable_iopoll_entity_t __ipc_server_vtable = {
    .write = NULL,
    .read  = __ipc_server_accept,
    .close = __ipc_server_close,
};

static const z_vtable_iopoll_entity_t __ipc_client_vtable = {
    .write = __ipc_client_write,
    .read  = __ipc_client_read,
    .close = __ipc_client_close,
};

/* ============================================================================
 *  IPC Public Methods
 */
int __z_ipc_plug (z_memory_t *memory,
                  z_iopoll_t *iopoll,
                  const z_ipc_protocol_t *proto,
                  unsigned int csize,
                  const void *address,
                  const void *service,
                  void *udata)
{
    z_ipc_server_t *server;

    if ((server = __ipc_server_alloc(memory, proto, address, service)) == NULL)
        return(1);

    server->iopoll = iopoll;
    server->udata = udata;
    server->csize = csize;

    if (z_iopoll_add(iopoll, Z_IOPOLL_ENTITY(server))) {
        __ipc_server_free(iopoll, server);
        return(-2);
    }

    return(0);
}
