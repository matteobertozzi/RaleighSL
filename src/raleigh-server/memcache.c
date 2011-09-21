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

#include <raleighfs/object/memcache.h>

#include <zcl/messageq.h>
#include <zcl/iopoll.h>
#include <zcl/memcmp.h>
#include <zcl/memcpy.h>
#include <zcl/strtol.h>
#include <zcl/chunkq.h>
#include <zcl/socket.h>
#include <zcl/debug.h>
#include <zcl/rpc.h>

#define __tokcmp(client, token, mem, memlen)                                  \
    z_chunkq_memcmp(&((client)->rdbuffer), (token)->offset, mem, memlen)

#define __tok2u32(token, value)                                               \
    z_chunkq_u32((token)->chunkq, (token)->offset, (token)->length, 10, value)

#define __tok2u64(token, value)                                               \
    z_chunkq_u64((token)->chunkq, (token)->offset, (token)->length, 10, value)

#define MAX_TOKENS                          8
#define COMMAND_TOKEN                       0
#define KEY_TOKEN                           1
#define KEY_MAX_LENGTH                      250
#define LINE_MAX_LENGTH                     (250 + KEY_MAX_LENGTH)

typedef int (*memcache_func_t)  (z_rpc_client_t *client,
                                 raleighfs_memcache_code_t method_code,
                                 const z_chunkq_extent_t *tokens,
                                 unsigned int ntokens,
                                 unsigned int nline);

struct memcache_command {
    const char *    name;
    unsigned int    length;
    raleighfs_memcache_code_t  code;
    int             rmline;
    memcache_func_t func;
};

#define __memcache_message_alloc(client, type)                                \
    z_message_alloc((client)->server->user_data, (client)->user_data, type)

static int __memcache_message_send (z_rpc_client_t *client,
                                    z_message_t *msg,
                                    z_message_func_t callback)
{
    z_rdata_t obj_name;
    z_rdata_from_blob(&obj_name, ":memcache:", 10);
    return(z_message_send(msg, &obj_name, callback, client));
}

static int __memcache_write_state (z_rpc_client_t *client,
                                   raleighfs_memcache_state_t state)
{
    unsigned int length;
    const char *message;

    switch (state) {
        case RALEIGHFS_MEMCACHE_BAD_CMDLINE:
            message = "CLIENT_ERROR bad command line format\r\n";
            length  = 38;
            break;
        case RALEIGHFS_MEMCACHE_OBJ_TOO_LARGE:
            message = "SERVER_ERROR object too large for cache\r\n";
            length  = 41;
            break;
        case RALEIGHFS_MEMCACHE_NO_MEMORY:
            message = "SERVER_ERROR out of memory storing object\r\n";
            length  = 43;
            break;
        case RALEIGHFS_MEMCACHE_INVALID_DELTA:
            message = "CLIENT_ERROR invalid numeric delta argument\r\n";
            length  = 45;
            break;
        case RALEIGHFS_MEMCACHE_NOT_NUMBER:
            message = "CLIENT_ERROR cannot increment or decrement non-numeric value\r\n";
            length  = 62;
            break;
        case RALEIGHFS_MEMCACHE_NOT_STORED:
            message = "NOT_STORED\r\n";
            length  = 12;
            break;
        case RALEIGHFS_MEMCACHE_EXISTS:
            message = "EXISTS\r\n";
            length = 8;
            break;
        case RALEIGHFS_MEMCACHE_OK:
            message = "OK\r\n";
            length = 4;
            break;
        case RALEIGHFS_MEMCACHE_STORED:
            message = "STORED\r\n";
            length = 8;
            break;
        case RALEIGHFS_MEMCACHE_DELETED:
            message = "DELETED\r\n";
            length = 9;
            break;
        case RALEIGHFS_MEMCACHE_NOT_FOUND:
            message = "NOT_FOUND\r\n";
            length = 11;
            break;
        case RALEIGHFS_MEMCACHE_VERSION:
            message = "VERSION 1.4.5\r\n";
            length = 15;
            break;
        case RALEIGHFS_MEMCACHE_ERROR_GENERIC:
        default:
            message = "ERROR\r\n";
            length  = 7;
            break;
    }

    return(z_rpc_write(client, message, length));
}

static void __complete_state (void *client, z_message_t *msg) {
    __memcache_write_state(Z_RPC_CLIENT(client), z_message_state(msg));
}

static int __memcache_send_state (z_rpc_client_t *client,
                                  raleighfs_memcache_state_t state)
{
    z_message_t *msg;

    msg = __memcache_message_alloc(client, Z_MESSAGE_BYPASS);
    z_message_set_state(msg, state);
    return(__memcache_message_send(client, msg, __complete_state));
}

static int __memcache_send_error (z_rpc_client_t *client,
                                  raleighfs_memcache_state_t state)
{
    z_chunkq_clear(&(client->rdbuffer));
    return(__memcache_send_state(client, state));
}

static void __set_noreply (z_rpc_client_t *client,
                           const z_chunkq_extent_t *tokens,
                           unsigned int ntokens)
{
    const z_chunkq_extent_t *tok;

    ntokens--;
    while (ntokens--) {
        tok = &(tokens[ntokens]);

        if (tok->length != 7)
            continue;

        if (!z_chunkq_memcmp(tok->chunkq, tok->offset, "noreply", 7)) {
            z_iopoll_entity_set_flag(client, Z_RPC_NOREPLY);
            break;
        }
    }
}

static int __get_header (char *buffer,
                         const z_stream_extent_t *key,
                         uint32_t flags,
                         uint32_t vlength,
                         uint64_t cas,
                         int use_cas)
{
    char *p = buffer;

    z_memcpy(p, "VALUE ", 6); p += 6;
    z_stream_read_extent(key, p); p += key->length;
    *p++ = ' ';
    p += z_u32tostr(flags, p, 16, 10);
    *p++ = ' ';
    p += z_u64tostr(vlength, p, 16, 10);
    if (use_cas) {
        *p++ = ' ';
        p += z_u64tostr(cas, p, 16, 10);
    }
    *p++ = '\r';
    *p++ = '\n';

    return(p - buffer);
}


static void __complete_get (void *user_data, z_message_t *msg) {
    z_rpc_client_t *client = Z_RPC_CLIENT(user_data);
    z_stream_extent_t value;
    z_stream_extent_t key;
    z_stream_t req_stream;
    z_stream_t res_stream;
    char text_number[16];
    char buffer[512];
    uint32_t klength;
    uint32_t vlength;
    uint32_t exptime;
    uint64_t number;
    uint32_t count;
    uint32_t state;
    uint32_t flags;
    uint64_t cas;
    int use_cas;
    int n;

    use_cas = (z_message_type(msg) == RALEIGHFS_MEMCACHE_GETS);

    z_message_response_stream(msg, &res_stream);
    z_message_request_stream(msg, &req_stream);
    z_stream_read_uint32(&req_stream, &count);

    while (count--) {
        z_stream_read_uint32(&req_stream, &klength);
        z_stream_set_extent(&req_stream, &key, req_stream.offset, klength);
        z_stream_seek(&req_stream, req_stream.offset + klength);

        z_stream_read_uint32(&res_stream, &state);
        if (state == RALEIGHFS_MEMCACHE_NOT_FOUND)
            continue;

        z_stream_read_uint32(&res_stream, &exptime);
        z_stream_read_uint32(&res_stream, &flags);
        z_stream_read_uint64(&res_stream, &cas);

        if (state == RALEIGHFS_MEMCACHE_NUMBER) {
            z_stream_read_uint64(&res_stream, &number);
            n = z_u64tostr(number, text_number, sizeof(text_number), 10);
            vlength = n;
            n = __get_header(buffer, &key, flags, vlength, cas, use_cas);
            z_rpc_write(client, buffer, n);
            z_rpc_write(client, text_number, vlength);
        } else {
            z_stream_read_uint32(&res_stream, &vlength);
            z_stream_set_extent(&res_stream, &value, res_stream.offset, vlength);
            z_stream_seek(&res_stream, res_stream.offset + vlength);

            n = __get_header(buffer, &key, flags, vlength, cas, use_cas);
            z_rpc_write(client, buffer, n);
            z_rpc_write_stream(client, &value);
        }

        z_rpc_write(client, "\r\n", 2);
    }

    z_rpc_write(client, "END\r\n", 5);

    z_message_free(msg);
}

static int __process_get (z_rpc_client_t *client,
                          raleighfs_memcache_code_t method_code,
                          const z_chunkq_extent_t *tokens,
                          unsigned int ntokens,
                          unsigned int nline)
{
    const z_chunkq_extent_t *key;
    z_message_t *msg;
    z_stream_t stream;

    if (ntokens < 3)
        return(__memcache_send_error(client, RALEIGHFS_MEMCACHE_ERROR_GENERIC));

    for (key = &(tokens[KEY_TOKEN]); key->length != 0; ++key) {
        if (key->length > KEY_MAX_LENGTH)
            return(__memcache_send_error(client, RALEIGHFS_MEMCACHE_BAD_CMDLINE));
    }

    /* Send GET Operation Command! */
    msg = __memcache_message_alloc(client, Z_MESSAGE_READ_REQUEST);
    z_message_set_type(msg, method_code);
    z_message_request_stream(msg, &stream);
    z_stream_write_uint32(&stream, (ntokens - 1) - KEY_TOKEN);

    for (key = &(tokens[KEY_TOKEN]); key->length != 0; ++key) {
        z_stream_write_uint32(&stream, key->length);
        z_stream_write_chunkq(&stream, key->chunkq, key->offset, key->length);
    }

    return(__memcache_message_send(client, msg, __complete_get));
}

static void __complete_update (void *user_data, z_message_t *msg) {
    __memcache_write_state(Z_RPC_CLIENT(user_data), z_message_state(msg));
    z_message_free(msg);
}

static int __process_update (z_rpc_client_t *client,
                             raleighfs_memcache_code_t code,
                             const z_chunkq_extent_t *tokens,
                             unsigned int ntokens,
                             unsigned int nline)
{
    const z_chunkq_extent_t *key;
    uint64_t req_cas_id = 0;
    z_message_t *msg;
    z_stream_t stream;
    uint32_t vlength;
    uint32_t exptime;
    uint32_t flags;
    long index;
    int res;

    if (ntokens < 6 || ntokens > 8)
        return(__memcache_send_error(client, RALEIGHFS_MEMCACHE_ERROR_GENERIC));

    /* check for noreply flag */
    __set_noreply(client, tokens, ntokens);

    key = &(tokens[KEY_TOKEN]);
    if (key->length > KEY_MAX_LENGTH)
        return(__memcache_send_error(client, RALEIGHFS_MEMCACHE_BAD_CMDLINE));

    if (!__tok2u32(&(tokens[4]), &vlength))
        return(__memcache_send_error(client, RALEIGHFS_MEMCACHE_BAD_CMDLINE));

    /* Data is not available yet */
    if (client->rdbuffer.size < (nline + vlength))
        return(1);

    if (!(__tok2u32(&(tokens[2]), &flags) && __tok2u32(&(tokens[3]), &exptime)))
        return(__memcache_send_error(client, RALEIGHFS_MEMCACHE_BAD_CMDLINE));

    if (code == RALEIGHFS_MEMCACHE_CAS && !__tok2u64(&(tokens[5]), &req_cas_id))
        return(__memcache_send_error(client, RALEIGHFS_MEMCACHE_BAD_CMDLINE));

    /* Send SET Operation Command! */
    msg = __memcache_message_alloc(client, Z_MESSAGE_WRITE_REQUEST);
    z_message_set_type(msg, code);
    z_message_request_stream(msg, &stream);
    z_stream_write_uint32(&stream, key->length);
    z_stream_write_uint32(&stream, vlength);
    z_stream_write_uint32(&stream, flags);
    z_stream_write_uint32(&stream, exptime);
    z_stream_write_uint64(&stream, req_cas_id);
    z_stream_write_chunkq(&stream, key->chunkq, key->offset, key->length);
    z_stream_write_chunkq(&stream, &(client->rdbuffer), nline, vlength);
    res = __memcache_message_send(client, msg, __complete_update);

    /* Remove request */
    z_chunkq_remove(&(client->rdbuffer), nline + vlength);
    if ((index = z_chunkq_indexof(&(client->rdbuffer), 0, "\n", 1)) >= 0)
        z_chunkq_remove(&(client->rdbuffer), index + 1);

    return(res);
}

static void __complete_arithmetic (void *user_data, z_message_t *msg) {
    z_rpc_client_t *client = Z_RPC_CLIENT(user_data);
    z_stream_t stream;

    if (z_message_state(msg) == RALEIGHFS_MEMCACHE_STORED) {
        char text_number[16];
        struct iovec iov[2];
        uint64_t number;
        int n;

        z_message_response_stream(msg, &stream);
        z_stream_read_uint64(&stream, &number);

        n = z_u64tostr(number, text_number, sizeof(text_number), 10);
        iov[0].iov_base = text_number;
        iov[0].iov_len = n;
        iov[1].iov_base = "\r\n";
        iov[1].iov_len = 2;
        z_rpc_writev(client, iov, 2);
    } else {
        __memcache_write_state(client, z_message_state(msg));
    }

    z_message_free(msg);
}

static int __process_arithmetic (z_rpc_client_t *client,
                                 raleighfs_memcache_code_t method_code,
                                 const z_chunkq_extent_t *tokens,
                                 unsigned int ntokens,
                                 unsigned int nline)
{
    const z_chunkq_extent_t *key;
    z_message_t *msg;
    z_stream_t stream;
    uint64_t delta;

    if (ntokens != 4 && ntokens != 5)
        return(__memcache_send_error(client, RALEIGHFS_MEMCACHE_ERROR_GENERIC));

    /* check for noreply flag */
    __set_noreply(client, tokens, ntokens);

    key = &(tokens[KEY_TOKEN]);
    if (key->length > KEY_MAX_LENGTH)
        return(__memcache_send_error(client, RALEIGHFS_MEMCACHE_BAD_CMDLINE));

    if (!__tok2u64(&(tokens[2]), &delta))
        return(__memcache_send_error(client, RALEIGHFS_MEMCACHE_INVALID_DELTA));

    /* Send Arithmetic Operation Command! */
    msg = __memcache_message_alloc(client, Z_MESSAGE_WRITE_REQUEST);
    z_message_set_type(msg, method_code);
    z_message_request_stream(msg, &stream);
    z_stream_write_uint64(&stream, delta);
    z_stream_write_uint32(&stream, key->length);
    z_stream_write_chunkq(&stream, key->chunkq, key->offset, key->length);
    return(__memcache_message_send(client, msg, __complete_arithmetic));
}

static void __complete_delete (void *user_data, z_message_t *msg) {
    z_rpc_client_t *client = Z_RPC_CLIENT(user_data);

    __memcache_write_state(client, z_message_state(msg));
    z_message_free(msg);
}

static int __process_delete (z_rpc_client_t *client,
                             raleighfs_memcache_code_t method_code,
                             const z_chunkq_extent_t *tokens,
                             unsigned int ntokens,
                             unsigned int nline)
{
    const z_chunkq_extent_t *key;
    z_message_t *msg;
    z_stream_t stream;

    if (ntokens < 3 || ntokens > 5)
        return(__memcache_send_error(client, RALEIGHFS_MEMCACHE_ERROR_GENERIC));

    /* check for noreply flag */
    __set_noreply(client, tokens, ntokens);

    key = &(tokens[KEY_TOKEN]);

    /* Send Delete Operation Command! */
    msg = __memcache_message_alloc(client, Z_MESSAGE_WRITE_REQUEST);
    z_message_set_type(msg, method_code);
    z_message_request_stream(msg, &stream);
    z_stream_write_uint32(&stream, key->length);
    z_stream_write_chunkq(&stream, key->chunkq, key->offset, key->length);
    return(__memcache_message_send(client, msg, __complete_delete));
}

static void __complete_flush_all (void *user_data, z_message_t *msg) {
    z_rpc_client_t *client = Z_RPC_CLIENT(user_data);
    __memcache_write_state(client, RALEIGHFS_MEMCACHE_OK);
    z_message_free(msg);
}

static int __process_flush_all (z_rpc_client_t *client,
                                raleighfs_memcache_code_t method_code,
                                const z_chunkq_extent_t *tokens,
                                unsigned int ntokens,
                                unsigned int nline)
{
    z_message_t *msg;

    if (ntokens < 2 || ntokens > 4)
         return(__memcache_send_error(client, RALEIGHFS_MEMCACHE_ERROR_GENERIC));

    /* check for noreply flag */
    __set_noreply(client, tokens, ntokens);

    /* Send Flush Operation Command! */
    msg = __memcache_message_alloc(client, Z_MESSAGE_WRITE_REQUEST);
    z_message_set_type(msg, method_code);
    return(__memcache_message_send(client, msg, __complete_flush_all));
}

static int __process_stats (z_rpc_client_t *client,
                            raleighfs_memcache_code_t method_code,
                            const z_chunkq_extent_t *tokens,
                            unsigned int ntokens,
                            unsigned int nline)
{
    if (ntokens < 2)
        return(__memcache_send_error(client, RALEIGHFS_MEMCACHE_ERROR_GENERIC));

    /*
        STAT pid 14414
        STAT uptime 2514
        STAT time 1307866248
        STAT version 1.4.5
        STAT pointer_size 32
        STAT rusage_user 20.225264
        STAT rusage_system 51.843240
        STAT curr_connections 5
        STAT total_connections 10
        STAT connection_structures 6
        STAT cmd_get 300000
        STAT cmd_set 600003
        STAT cmd_flush 0
        STAT get_hits 300000
        STAT get_misses 0
        STAT delete_misses 0
        STAT delete_hits 600000
        STAT incr_misses 0
        STAT incr_hits 0
        STAT decr_misses 0
        STAT decr_hits 0
        STAT cas_misses 0
        STAT cas_hits 0
        STAT cas_badval 0
        STAT auth_cmds 0
        STAT auth_errors 0
        STAT bytes_read 34206979
        STAT bytes_written 21503393
        STAT limit_maxbytes 67108864
        STAT accepting_conns 1
        STAT listen_disabled_num 0
        STAT threads 4
        STAT conn_yields 0
        STAT bytes 109
        STAT curr_items 1
        STAT total_items 600003
        STAT evictions 0
        STAT reclaimed 0
        END
    */
    return(__memcache_send_state(client, RALEIGHFS_MEMCACHE_ERROR_GENERIC));
}

static int __process_version (z_rpc_client_t *client,
                              raleighfs_memcache_code_t method_code,
                              const z_chunkq_extent_t *tokens,
                              unsigned int ntokens,
                              unsigned int nline)
{
    if (ntokens != 2)
        return(__memcache_send_error(client, RALEIGHFS_MEMCACHE_ERROR_GENERIC));

    return(__memcache_send_state(client, RALEIGHFS_MEMCACHE_VERSION));
}

static int __process_verbosity (z_rpc_client_t *client,
                                raleighfs_memcache_code_t method_code,
                                const z_chunkq_extent_t *tokens,
                                unsigned int ntokens,
                                unsigned int nline)
{
    uint32_t level;

    if (ntokens < 3 || ntokens > 4)
        return(__memcache_send_error(client, RALEIGHFS_MEMCACHE_ERROR_GENERIC));

    /* check for noreply flag */
    __set_noreply(client, tokens, ntokens);

    __tok2u32(&(tokens[1]), &level);

    return(__memcache_send_state(client, RALEIGHFS_MEMCACHE_OK));
}

static int __process_quit (z_rpc_client_t *client,
                           raleighfs_memcache_code_t method_code,
                           const z_chunkq_extent_t *tokens,
                           unsigned int ntokens,
                           unsigned int nline)
{
    if (ntokens != 2)
        return(__memcache_send_error(client, RALEIGHFS_MEMCACHE_ERROR_GENERIC));
    return(-1);
}

static const struct memcache_command __memcache_commands[] = {
    { "get",       3, RALEIGHFS_MEMCACHE_GET,       1, __process_get },
    { "bget",      4, RALEIGHFS_MEMCACHE_BGET,      1, __process_get },
    { "add",       3, RALEIGHFS_MEMCACHE_ADD,       0, __process_update },
    { "set",       3, RALEIGHFS_MEMCACHE_SET,       0, __process_update },
    { "replace",   7, RALEIGHFS_MEMCACHE_REPLACE,   0, __process_update },
    { "prepend",   7, RALEIGHFS_MEMCACHE_PREPEND,   0, __process_update },
    { "append",    6, RALEIGHFS_MEMCACHE_APPEND,    0, __process_update },
    { "cas",       3, RALEIGHFS_MEMCACHE_CAS,       0, __process_update },
    { "incr",      4, RALEIGHFS_MEMCACHE_INCR,      1, __process_arithmetic },
    { "decr",      4, RALEIGHFS_MEMCACHE_DECR,      1, __process_arithmetic },
    { "gets",      4, RALEIGHFS_MEMCACHE_GETS,      1, __process_get },
    { "delete",    6, RALEIGHFS_MEMCACHE_DELETE,    1, __process_delete },
    { "flush_all", 9, RALEIGHFS_MEMCACHE_FLUSH_ALL, 1, __process_flush_all },
    { "stats",     5, 0,                            1, __process_stats },
    { "version",   7, 0,                            1, __process_version },
    { "verbosity", 9, 0,                            1, __process_verbosity },
    { "quit",      4, 0,                            1, __process_quit },
    { NULL,        0, 0,                            1, NULL },
};

/* ============================================================================
 *  Memcache Protocol Methods
 */
static int __memcache_bind (const void *hostname, const void *service) {
    int socket;

    if ((socket = z_socket_tcp_bind(hostname, service, NULL)) < 0)
        return(-1);

    Z_PRINT_DEBUG(" - Memcached Interface up and running on %s:%s...\n",
                  (const char *)hostname, (const char *)service);

    return(socket);
}

static int __memcache_accept (z_rpc_server_t *server) {
    struct sockaddr_storage address;
    char ip[INET6_ADDRSTRLEN];
    int csock;

    if ((csock = z_socket_tcp_accept(Z_IOPOLL_ENTITY_FD(server), NULL)) < 0)
        return(-1);

    z_socket_tcp_set_nodelay(csock);

    z_socket_address(csock, &address);
    z_socket_str_address(ip, INET6_ADDRSTRLEN, (struct sockaddr *)&address);
    Z_PRINT_DEBUG(" - Memcached Accept %d %s.\n", csock, ip);

    return(csock);
}

static int __memcache_connected (z_rpc_client_t *client) {
    z_message_source_t *source;

    if ((source = z_message_source_alloc((client)->server->user_data)) == NULL)
        return(1);

    client->user_data = source;
    return(0);
}

static void __memcache_disconnected (z_rpc_client_t *client) {
    z_message_source_free(client->user_data);
}

static unsigned int __tokenize_command (z_rpc_client_t *client,
                                        unsigned int nline,
                                        z_chunkq_extent_t *tokens,
                                        unsigned int max_tokens)
{
    z_chunkq_extent_t *token = tokens;
    unsigned int ntokens = 0;
    unsigned int offset = 0;

    while (z_rpc_tokenize(client, token, offset)) {
        offset = token->offset + token->length + 1;

        if (offset >= nline)
            break;

        token++;
        if (++ntokens == max_tokens)
            break;
    }

    token->offset = offset;
    token->length = 0;

    return(ntokens + 1);
}

static int __memcache_process_line (z_rpc_client_t *client,
                                    unsigned int nline)
{
    z_chunkq_extent_t tokens[MAX_TOKENS];
    const struct memcache_command *p;
    unsigned int ntokens;
    int result;

    ntokens = __tokenize_command(client, nline, tokens, MAX_TOKENS);
    for (p = __memcache_commands; p->name != NULL; ++p) {
        if (p->length != tokens[COMMAND_TOKEN].length)
            continue;

        if (__tokcmp(client, &(tokens[COMMAND_TOKEN]), p->name, p->length))
            continue;

        z_iopoll_entity_reset_flags(client);
        if ((result = p->func(client, p->code, tokens, ntokens, nline)))
            return(result);

        if (p->rmline)
            z_chunkq_remove(&(client->rdbuffer), nline);

        return(0);
    }

    /* Invalid Command */
    return(__memcache_send_error(client, RALEIGHFS_MEMCACHE_ERROR_GENERIC));
}

z_rpc_protocol_t memcache_protocol = {
    .bind           = __memcache_bind,
    .accept         = __memcache_accept,
    .connected      = __memcache_connected,
    .disconnected   = __memcache_disconnected,
    .process        = NULL,
    .process_line   = __memcache_process_line,
};

