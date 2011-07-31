/*
 *   Copyright 2011 Matteo Bertozzi
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

#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdio.h>

#include <zcl/rpc.h>

#define __RPC_READ_BUFSIZE              (4096 - sizeof(z_chunkq_node_t) + 1)
#define __RPC_WRITE_BUFSIZE             (4096 - sizeof(z_chunkq_node_t) + 1)

static z_iopoll_entity_plug_t __rpc_server_ioplug;
static z_iopoll_entity_plug_t __rpc_client_ioplug;

static z_rpc_server_t *__rpc_server_alloc (z_memory_t *memory,
                                           z_rpc_protocol_t *proto,
                                           const void *hostname,
                                           const void *service)
{
    z_rpc_server_t *server;
    int socket;

    if ((server = z_memory_struct_alloc(memory, z_rpc_server_t)) == NULL)
        return(NULL);

    if ((socket = proto->bind(hostname, service)) < 0) {
        z_memory_struct_free(memory, z_rpc_server_t, server);
        return(NULL);
    }

    z_iopoll_entity_init(server, socket, &__rpc_server_ioplug);
    server->protocol = proto;
    server->memory = memory;

    return(server);
}

static void __rpc_server_free (z_iopoll_t *iopoll, z_rpc_server_t *server) {
    close(Z_IOPOLL_ENTITY_FD(server));
    z_memory_struct_free(server->memory, z_rpc_server_t, server);
}

static z_rpc_client_t *__rpc_client_alloc (z_rpc_server_t *server, int csock) {
    z_rpc_client_t *client;
    z_memory_t *memory;

    memory = server->memory;
    if ((client = z_memory_struct_alloc(memory, z_rpc_client_t)) == NULL)
        return(NULL);

    z_iopoll_entity_init(client, csock, &__rpc_client_ioplug);

    /* Allocate Read Buffer */
    if (!z_chunkq_alloc(&(client->rdbuffer), memory, __RPC_READ_BUFSIZE)) {
        close(csock);
        z_memory_struct_free(memory, z_rpc_client_t, client);
        return(NULL);
    }

    /* Allocate Write Buffer */
    if (!z_chunkq_alloc(&(client->wrbuffer), memory, __RPC_WRITE_BUFSIZE)) {
        close(csock);
        z_chunkq_free(&(client->rdbuffer));
        z_memory_struct_free(memory, z_rpc_client_t, client);
        return(NULL);
    }

    client->server = server;
    gettimeofday(&(client->time), NULL);

    return(client);
}

static void __rpc_client_free (z_iopoll_t *iopoll, z_rpc_client_t *client) {
    z_rpc_server_t *server = client->server;

#if 1
    struct timeval now, r;
    gettimeofday(&now, NULL);
    timersub(&now, &(client->time), &r);
    fprintf(stderr, " -----> Client online time %.8f\n",
            r.tv_sec + (double)r.tv_usec / 1000000);
#endif
    close(Z_IOPOLL_ENTITY_FD(client));

    if (server->protocol->disconnected != NULL)
        server->protocol->disconnected(client);

    z_chunkq_free(&(client->rdbuffer));
    z_chunkq_free(&(client->wrbuffer));
    z_memory_struct_free(server->memory, z_rpc_client_t, client);
}

static int __rpc_server_accept (z_iopoll_t *iopoll, z_iopoll_entity_t *entity) {
    z_rpc_server_t *server = Z_RPC_SERVER(entity);
    z_rpc_client_t *client;
    int csock;

    if ((csock = server->protocol->accept(server)) < 0)
        return(-1);

    if ((client = __rpc_client_alloc(server, csock)) == NULL) {
        close(csock);
        return(-2);
    }

    if (server->protocol->connected != NULL) {
        if (server->protocol->connected(client) < 0) {
            __rpc_client_free(server->iopoll, client);
            return(-3);
        }
    }

    if (z_iopoll_add(server->iopoll, Z_IOPOLL_ENTITY(client))) {
        __rpc_client_free(server->iopoll, client);
        return(-4);
    }

    return(0);
}

static int __rpc_client_read (z_iopoll_t *iopoll, z_iopoll_entity_t *entity) {
    z_rpc_client_t *client = Z_RPC_CLIENT(entity);
    int ret;

    z_chunkq_append_fetch_fd(&(client->rdbuffer), Z_IOPOLL_ENTITY_FD(client));

    while (client->rdbuffer.size > 0) {
        if ((ret = client->server->protocol->process(client)) < 0) {
            z_iopoll_remove(client->server->iopoll, Z_IOPOLL_ENTITY(client));
            return(-1);
        }

        if (ret != 0)
            break;
    }

    return(0);
}

static int __rpc_client_write (z_iopoll_t *iopoll, z_iopoll_entity_t *entity) {
    z_rpc_client_t *client = Z_RPC_CLIENT(entity);

    if (client->wrbuffer.size > 0) {
        if (fcntl(entity->fd, F_GETFL) < 0)
            return(0);

        z_chunkq_remove_push_fd(&(client->wrbuffer), entity->fd);
    }

    if (client->wrbuffer.size == 0)
        z_iopoll_entity_set_writeable(iopoll, entity, 0);

    return(0);
}

static z_iopoll_entity_plug_t __rpc_server_ioplug = {
    .write = NULL,
    .read  = __rpc_server_accept,
    .free  = (z_mem_free_t)__rpc_server_free,
};

static z_iopoll_entity_plug_t __rpc_client_ioplug = {
    .write = __rpc_client_write,
    .read  = __rpc_client_read,
    .free  = (z_mem_free_t)__rpc_client_free,
};

int z_rpc_plug (z_memory_t *memory,
                z_iopoll_t *iopoll,
                z_rpc_protocol_t *proto,
                const void *hostname,
                const void *service,
                void *user_data)
{
    z_rpc_server_t *server;

    if ((server = __rpc_server_alloc(memory, proto, hostname, service)) == NULL)
        return(1);

    server->iopoll = iopoll;
    if (z_iopoll_add(iopoll, Z_IOPOLL_ENTITY(server))) {
        __rpc_server_free(iopoll, server);
        return(2);
    }

    server->user_data = user_data;
    return(0);
}

int z_rpc_write (z_rpc_client_t *client,
                 const void *data,
                 unsigned int n)
{
    if (z_iopoll_entity_flag(client, Z_RPC_NOREPLY))
        return(0);

    z_chunkq_append(&(client->wrbuffer), data, n);
    z_iopoll_entity_set_writeable(client->server->iopoll,
                                  Z_IOPOLL_ENTITY(client), 1);

    return(0);
}

int z_rpc_writev (z_rpc_client_t *client,
                  const struct iovec *iov,
                  unsigned int iovecnt)
{
    unsigned int i;

    if (z_iopoll_entity_flag(client, Z_RPC_NOREPLY))
        return(0);

    for (i = 0; i < iovecnt; ++i)
        z_chunkq_append(&(client->wrbuffer), iov[i].iov_base, iov[i].iov_len);

    z_iopoll_entity_set_writeable(client->server->iopoll,
                                  Z_IOPOLL_ENTITY(client), 1);
    return(0);
}

int z_rpc_write_chunk (z_rpc_client_t *client,
                       const z_chunkq_extent_t *extent)
{
    unsigned int n, rd, offset;
    char buffer[1024];

    if (z_iopoll_entity_flag(client, Z_RPC_NOREPLY))
        return(0);

    n = extent->length;
    offset = extent->offset;
    while (n > 0) {
        rd = (n > sizeof(buffer)) ? sizeof(buffer) : n;

        rd = z_chunkq_read(extent->chunkq, offset, buffer, rd);
        z_chunkq_append(&(client->wrbuffer), buffer, rd);

        n -= rd;
        offset += rd;
    }

    z_iopoll_entity_set_writeable(client->server->iopoll,
                                  Z_IOPOLL_ENTITY(client), 1);
    return(0);
}

int z_rpc_write_stream (z_rpc_client_t *client,
                        const z_stream_extent_t *extent)
{
    unsigned int n, rd, offset;
    char buffer[1024];

    if (z_iopoll_entity_flag(client, Z_RPC_NOREPLY))
        return(0);

    n = extent->length;
    offset = extent->offset;
    while (n > 0) {
        rd = (n > sizeof(buffer)) ? sizeof(buffer) : n;

        rd = z_stream_pread(extent->stream, offset, buffer, rd);
        z_chunkq_append(&(client->wrbuffer), buffer, rd);

        n -= rd;
        offset += rd;
    }

    z_iopoll_entity_set_writeable(client->server->iopoll,
                                  Z_IOPOLL_ENTITY(client), 1);
    return(0);
}

int z_rpc_tokenize (z_rpc_client_t *client,
                    z_chunkq_extent_t *extent,
                    unsigned int offset)
{
    return(z_chunkq_tokenize(&(client->rdbuffer), extent,
                             offset, " \t\r\n\v\f", 6));
}

int z_rpc_tokenize_line (z_rpc_client_t *client,
                         z_chunkq_extent_t *extent,
                         unsigned int offset)
{
    return(z_chunkq_tokenize(&(client->rdbuffer), extent, offset, "\r\n", 2));
}

unsigned int z_rpc_has_line (z_rpc_client_t *client, unsigned int offset) {
    long index;

    if ((index = z_chunkq_indexof(&(client->rdbuffer), offset, "\n", 1)) < 0)
        return(0);

    return(index);
}

