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

#include <zcl/iopoll.h>
#include <zcl/chunkq.h>
#include <zcl/socket.h>
#include <zcl/rpc.h>

#include <stdio.h>

#include "zleveldb.h"

#define __LEVELDB(client)           Z_LEVELDB((client)->server->user_data)

#define __tokcmp(client, token, mem, memlen)                                  \
    z_chunkq_memcmp(&((client)->rdbuffer), (token)->offset, mem, memlen)

#define __tokread(token, buffer)                                              \
    z_chunkq_read((token)->chunkq, (token)->offset, buffer, (token)->length)

#define __tok2u32(token, value)                                               \
    z_chunkq_u32((token)->chunkq, (token)->offset, (token)->length, 10, value)

#define __tok2u64(token, value)                                               \
    z_chunkq_u64((token)->chunkq, (token)->offset, (token)->length, 10, value)

#define MAX_TOKENS                          32
#define COMMAND_TOKEN                       0
#define KEY_TOKEN                           1
#define KEY_MAX_LENGTH                      250
#define LINE_MAX_LENGTH                     (250 + KEY_MAX_LENGTH)

typedef enum leveldb_errno {
    LEVELDB_ERRNO_GENERIC,
    LEVELDB_ERRNO_BAD_CMDLINE,
} leveldb_errno_t;

typedef enum leveldb_message {
    LEVELDB_MESSAGE_OK,
    LEVELDB_MESSAGE_STORED,
    LEVELDB_MESSAGE_DELETED,
    LEVELDB_MESSAGE_NOT_FOUND,
} leveldb_message_t;

typedef int (*leveldb_func_t)  (z_rpc_client_t *client,
                                 const z_chunkq_extent_t *tokens,
                                 unsigned int ntokens,
                                 unsigned int nline);

struct leveldb_command {
    const char *    name;
    unsigned int    length;
    int             rmline;
    leveldb_func_t  func;
};

static int __leveldb_write_error (z_rpc_client_t *client,
                                   leveldb_errno_t errno)
{
    unsigned int length;
    const char *message;

    switch (errno) {
        case LEVELDB_ERRNO_BAD_CMDLINE:
            message = "CLIENT_ERROR bad command line format\r\n";
            length  = 38;
            break;
        case LEVELDB_ERRNO_GENERIC:
        default:
            message = "ERROR\r\n";
            length  = 7;
            break;
    }

    z_rpc_write(client, message, length);
    return(errno);
}

static int __leveldb_write_message (z_rpc_client_t *client,
                                     leveldb_message_t msg)
{
    unsigned int length;
    const char *message;

    switch (msg) {
        case LEVELDB_MESSAGE_OK:
            message = "OK\r\n";
            length = 4;
            break;
        case LEVELDB_MESSAGE_STORED:
            message = "STORED\r\n";
            length = 8;
            break;
        case LEVELDB_MESSAGE_DELETED:
            message = "DELETED\r\n";
            length = 9;
            break;
        case LEVELDB_MESSAGE_NOT_FOUND:
            message = "NOT_FOUND\r\n";
            length = 11;
            break;
        default:
            message = "DONT KNOW\r\n";
            length = 11;
            break;
    }

    z_rpc_write(client, message, length);
    return(0);
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

static int __process_get (z_rpc_client_t *client,
                          const z_chunkq_extent_t *tokens,
                          unsigned int ntokens,
                          unsigned int nline)
{
    const z_chunkq_extent_t *key;
    char buf[KEY_MAX_LENGTH];
    size_t vlen;
    void *v;
    int n;

    if (ntokens < 3)
        return(__leveldb_write_error(client, LEVELDB_ERRNO_GENERIC));

    for (key = &(tokens[KEY_TOKEN]); key->length != 0; ++key) {
        if (key->length > KEY_MAX_LENGTH)
            return(__leveldb_write_error(client, LEVELDB_ERRNO_BAD_CMDLINE));
    }

    /* Send GET Operation Command! */
    for (key = &(tokens[KEY_TOKEN]); key->length != 0; ++key) {
        __tokread(key, buf);

        if ((v = z_leveldb_get(__LEVELDB(client), buf, key->length, &vlen))) {
            z_rpc_write(client, "VALUE ", 6);
            z_rpc_write(client, buf, key->length);
            n = snprintf(buf, KEY_MAX_LENGTH, " %lu\r\n", vlen);
            z_rpc_write(client, buf, n);
            z_rpc_write(client, v, vlen);
            z_rpc_write(client, "\r\n", 2);

            free(v);
        }
    }
    z_rpc_write(client, "END\r\n", 5);

    return(0);
}

static int __process_set (z_rpc_client_t *client,
                          const z_chunkq_extent_t *tokens,
                          unsigned int ntokens,
                          unsigned int nline)
{
    char key[KEY_MAX_LENGTH];
    uint32_t vlen;
    long index;
    void *v;

    if (ntokens != 4)
        return(__leveldb_write_error(client, LEVELDB_ERRNO_GENERIC));

    /* check for noreply flag */
    __set_noreply(client, tokens, ntokens);

    if (tokens[KEY_TOKEN].length > KEY_MAX_LENGTH)
        return(__leveldb_write_error(client, LEVELDB_ERRNO_BAD_CMDLINE));

    if (!__tok2u32(&(tokens[KEY_TOKEN + 1]), &vlen))
        return(__leveldb_write_error(client, LEVELDB_ERRNO_BAD_CMDLINE));

    /* Data is not available yet */
    if (client->rdbuffer.size < (nline + vlen))
        return(1);

    /* Allocate and read data */
    if ((v = malloc(vlen)) == NULL)
        return(__leveldb_write_error(client, LEVELDB_ERRNO_GENERIC));

    z_chunkq_read(&(client->rdbuffer), nline, v, vlen);

    /* Send SET Operation Command! */
    __tokread(&(tokens[KEY_TOKEN]), key);

    if (z_leveldb_put(__LEVELDB(client), key,tokens[KEY_TOKEN].length, v, vlen))
        return(__leveldb_write_error(client, LEVELDB_ERRNO_GENERIC));

    free(v);

    /* Remove request */
    z_chunkq_remove(&(client->rdbuffer), nline + vlen);
    if ((index = z_chunkq_indexof(&(client->rdbuffer), 0, "\n", 1)) >= 0)
        z_chunkq_remove(&(client->rdbuffer), index + 1);

    return(__leveldb_write_message(client, LEVELDB_MESSAGE_STORED));
}

static int __process_delete (z_rpc_client_t *client,
                             const z_chunkq_extent_t *tokens,
                             unsigned int ntokens,
                             unsigned int nline)
{
    char key[KEY_MAX_LENGTH];

    if (ntokens < 3 || ntokens > 5)
        return(__leveldb_write_error(client, LEVELDB_ERRNO_GENERIC));

    /* check for noreply flag */
    __set_noreply(client, tokens, ntokens);

    /* Delete key */
    __tokread(&(tokens[KEY_TOKEN]), key);
    if (z_leveldb_delete(__LEVELDB(client), key, tokens[KEY_TOKEN].length))
        return(__leveldb_write_message(client, LEVELDB_MESSAGE_NOT_FOUND));

    return(__leveldb_write_message(client, LEVELDB_MESSAGE_DELETED));
}

static int __process_flush_all (z_rpc_client_t *client,
                                const z_chunkq_extent_t *tokens,
                                unsigned int ntokens,
                                unsigned int nline)
{
    leveldb_writebatch_t *wbatch;
    leveldb_iterator_t *iter;
    const char *key;
    size_t keylen;

    if (ntokens < 2 || ntokens > 4)
         return(__leveldb_write_error(client, LEVELDB_ERRNO_GENERIC));

    /* check for noreply flag */
    __set_noreply(client, tokens, ntokens);

    if ((iter = z_leveldb_iter(__LEVELDB(client))) == NULL)
         return(__leveldb_write_error(client, LEVELDB_ERRNO_GENERIC));

    /* loop and delete all keys */
    wbatch = leveldb_writebatch_create();
    leveldb_iter_seek_to_first(iter);
    while (leveldb_iter_valid(iter)) {
        key = leveldb_iter_key(iter, &keylen);
        leveldb_writebatch_delete(wbatch, key, keylen);
        leveldb_iter_next(iter);
    }
    z_leveldb_write(__LEVELDB(client), wbatch);
    leveldb_writebatch_destroy(wbatch);

    return(__leveldb_write_message(client, LEVELDB_MESSAGE_OK));
}

static int __process_stats (z_rpc_client_t *client,
                            const z_chunkq_extent_t *tokens,
                            unsigned int ntokens,
                            unsigned int nline)
{
    char *stats;

    if (ntokens < 2)
        return(__leveldb_write_error(client, LEVELDB_ERRNO_GENERIC));

    if ((stats = z_leveldb_stats(__LEVELDB(client))) == NULL)
        return(__leveldb_write_error(client, LEVELDB_ERRNO_GENERIC));

    z_rpc_write(client, stats, z_strlen(stats));
    free(stats);

    z_rpc_write(client, "\r\nEND\r\n", 7);

    return(0);
}

static int __process_quit (z_rpc_client_t *client,
                           const z_chunkq_extent_t *tokens,
                           unsigned int ntokens,
                           unsigned int nline)
{
    if (ntokens != 2)
        return(__leveldb_write_error(client, LEVELDB_ERRNO_GENERIC));
    return(-1);
}

static struct leveldb_command __leveldb_commands[] = {
    { "set",       3, 0, __process_set },
    { "get",       3, 1, __process_get },
    { "delete",    6, 1, __process_delete },
    { "flush_all", 9, 1, __process_flush_all },
    { "stats",     5, 1, __process_stats },
    { "quit",      4, 1, __process_quit },
    { NULL,        0, 1, NULL },
};

/* ============================================================================
 *  Memcache Protocol Methods
 */
static int __leveldb_bind (const void *hostname, const void *service) {
    int socket;

    if ((socket = z_socket_tcp_bind((const char *)hostname, (const char *)service, NULL)) < 0)
        return(-1);

    fprintf(stderr, " - LevelDB Interface up and running on %s:%s...\n",
            (const char *)hostname, (const char *)service);

    return(socket);
}

static int __leveldb_accept (z_rpc_server_t *server) {
    struct sockaddr_storage address;
    char ip[INET6_ADDRSTRLEN];
    int csock;

    if ((csock = z_socket_tcp_accept(Z_IOPOLL_ENTITY_FD(server), NULL)) < 0)
        return(-1);

    z_socket_tcp_set_nodelay(csock);

    z_socket_address(csock, &address);
    z_socket_str_address(ip, INET6_ADDRSTRLEN, (struct sockaddr *)&address);
    fprintf(stderr, " - LevelDB Accept %d %s.\n", csock, ip);

    return(csock);
}

static int __leveldb_connected (z_rpc_client_t *client) {
    client->user_data = NULL; /* message source */
    return(0);
}

static void __leveldb_disconnected (z_rpc_client_t *client) {
    client->user_data = NULL; /* message source */
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

static int __leveldb_process_line (z_rpc_client_t *client,
                                    unsigned int nline)
{
    z_chunkq_extent_t tokens[MAX_TOKENS];
    struct leveldb_command *p;
    unsigned int ntokens;
    int result;

    ntokens = __tokenize_command(client, nline, tokens, MAX_TOKENS);
    for (p = __leveldb_commands; p->name != NULL; ++p) {
        if (p->length != tokens[COMMAND_TOKEN].length)
            continue;

        if (__tokcmp(client, &(tokens[COMMAND_TOKEN]), p->name, p->length))
            continue;

        z_iopoll_entity_reset_flags(client);
        if ((result = p->func(client, tokens, ntokens, nline)))
            return(result);

        if (p->rmline)
            z_chunkq_remove(&(client->rdbuffer), nline);

        return(0);
    }

    /* Invalid Command */
    if (!z_chunkq_startswith(&(client->rdbuffer), 0, "\r", 1) &&
        !z_chunkq_startswith(&(client->rdbuffer), 0, "\n", 1))
    {
        __leveldb_write_error(client, LEVELDB_ERRNO_GENERIC);
    }

    z_chunkq_remove(&(client->rdbuffer), nline);
    return(0);
}

static int __leveldb_process (z_rpc_client_t *client) {
    unsigned int nline;

    if ((nline = z_rpc_has_line(client, 0)) > 0)
        return(__leveldb_process_line(client, nline + 1));

    return(1);
}

z_rpc_protocol_t leveldb_protocol = {
    __leveldb_bind,
    __leveldb_accept,
    __leveldb_connected,
    __leveldb_disconnected,
    __leveldb_process,
};

