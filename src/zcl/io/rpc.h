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

#ifndef _Z_RPC_H_
#define _Z_RPC_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <sys/uio.h>

#include <zcl/iopoll.h>
#include <zcl/chunkq.h>
#include <zcl/stream.h>

#define Z_RPC_SERVER(x)                   Z_CAST(z_rpc_server_t, x)
#define Z_RPC_CLIENT(x)                   Z_CAST(z_rpc_client_t, x)

#define Z_RPC_NOREPLY                     (1)

Z_TYPEDEF_CONST_STRUCT(z_rpc_protocol)
Z_TYPEDEF_STRUCT(z_rpc_server)
Z_TYPEDEF_STRUCT(z_rpc_client)

struct z_rpc_server {
    Z_IOPOLL_ENTITY_TYPE

    z_rpc_protocol_t *protocol;
    z_memory_t *      memory;
    z_iopoll_t *      iopoll;
    void *            user_data;
};

struct z_rpc_client {
    Z_IOPOLL_ENTITY_TYPE

    const z_rpc_server_t *server;
    void *                user_data;

    z_chunkq_t      rdbuffer;
    z_chunkq_t      wrbuffer;

    struct timeval  time;
};

struct z_rpc_protocol {
    int     (*bind)             (const void *host, const void *service);
    int     (*accept)           (z_rpc_server_t *server);
    int     (*connected)        (z_rpc_client_t *client);
    void    (*disconnected)     (z_rpc_client_t *client);
    int     (*process)          (z_rpc_client_t *client);
    int     (*process_line)     (z_rpc_client_t *client, unsigned int n);
};

int             z_rpc_plug            (z_memory_t *memory,
                                       z_iopoll_t *iopoll,
                                       z_rpc_protocol_t *proto,
                                       const void *hostname,
                                       const void *service,
                                       void *user_data);

int             z_rpc_write           (z_rpc_client_t *client,
                                       const void *data,
                                       unsigned int n);
int             z_rpc_writev          (z_rpc_client_t *client,
                                       const struct iovec *iov,
                                       unsigned int iovecnt);
int             z_rpc_write_chunk     (z_rpc_client_t *client,
                                       const z_chunkq_extent_t *extent);
int             z_rpc_write_stream    (z_rpc_client_t *client,
                                       const z_stream_extent_t *extent);

int             z_rpc_tokenize        (z_rpc_client_t *client,
                                       z_chunkq_extent_t *extent,
                                       unsigned int offset);
unsigned int    z_rpc_ntokenize       (z_rpc_client_t *client,
                                       z_chunkq_extent_t *tokens,
                                       unsigned int max_tokens,
                                       unsigned int offset,
                                       unsigned int length);
int             z_rpc_tokenize_line   (z_rpc_client_t *client,
                                       z_chunkq_extent_t *extent,
                                       unsigned int offset);
unsigned int    z_rpc_ntokenize_line  (z_rpc_client_t *client,
                                       z_chunkq_extent_t *tokens,
                                       unsigned int max_tokens,
                                       unsigned int offset,
                                       unsigned int length);

unsigned int    z_rpc_has_line        (z_rpc_client_t *client,
                                       unsigned int offset);

__Z_END_DECLS__

#endif /* !_Z_RPC_H_ */

