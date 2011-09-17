#include <raleighfs/object/memcache.h>
#include <raleighfs/object/counter.h>
#include <raleighfs/object/deque.h>
#include <raleighfs/object/sset.h>
#include <raleighfs/raleighfs.h>

#include <zcl/messageq.h>
#include <zcl/snprintf.h>
#include <zcl/iopoll.h>
#include <zcl/memcmp.h>
#include <zcl/memcpy.h>
#include <zcl/strtol.h>
#include <zcl/chunkq.h>
#include <zcl/socket.h>
#include <zcl/debug.h>
#include <zcl/rpc.h>

#include <stdio.h>

#define __tokcmp(token, mem, memlen)                                          \
    z_chunkq_memcmp((token)->chunkq, (token)->offset, mem, memlen)

#define __tokocmp(token, toff, mem, memlen)                                   \
    z_chunkq_memcmp((token)->chunkq, (token)->offset + (toff), mem, memlen)

#define __tok2u32(token, value)                                               \
    z_chunkq_u32((token)->chunkq, (token)->offset, (token)->length, 10, value)

#define __tok2u64(token, value)                                               \
    z_chunkq_u64((token)->chunkq, (token)->offset, (token)->length, 10, value)

#define OBJECT_MAX_NAME                     (2048)

#define KEY_MAX_LENGTH                      (250)
#define LINE_MAX_LENGTH                     (250 + KEY_MAX_LENGTH)

typedef enum rpc_token_type {
    NAMESPACE_TOKEN = 0,
    COMMAND_TOKEN   = 1,
    OBJECT_TOKEN    = 2,
    ARG0_TOKEN      = 3,
    ARG1_TOKEN      = 4,
    ARG2_TOKEN      = 5,
    ARG3_TOKEN      = 6,
    ARG4_TOKEN      = 7,
    ARG5_TOKEN      = 8,
    ARG6_TOKEN      = 9,
    ARG7_TOKEN      = 10,
    ARG8_TOKEN      = 11,
    ARG9_TOKEN      = 12,
    MAX_TOKENS      = 13,
} rpc_token_type_t;

typedef enum rpc_errno {
    RPC_ERRNO_GENERIC,
    RPC_ERRNO_BAD_LINE,
    RPC_ERRNO_INVALID_COMMAND,
} rpc_errno_t;

typedef int     (*rpc_func_t)       (z_rpc_client_t *client,
                                     unsigned int method_code,
                                     const z_chunkq_extent_t *tokens,
                                     unsigned int ntokens,
                                     unsigned int nline);

struct rpc_command_group {
    const char * name;
    unsigned int length;
    const struct rpc_command *commands;
};

struct rpc_command {
    const char * name;
    unsigned int length;
    unsigned int code;
    int          rmline;
    rpc_func_t   func;
    unsigned int subs;
};

#define __rpc_message_alloc(client, type)                                     \
    z_message_alloc((client)->server->user_data, (client)->user_data, type)

static int __rpc_message_send (z_rpc_client_t *client,
                               const z_chunkq_extent_t *tokens,
                               z_message_t *msg,
                               z_message_func_t callback)
{
    z_rdata_t object_name;
    z_rdata_from_chunkq(&object_name, &(tokens[OBJECT_TOKEN]));
    return(z_message_send(msg, &object_name, callback, client));
}

static void __rpc_client_remove_to_eol (z_rpc_client_t *client, unsigned int n)
{
    long index;

    /* Remove n bytes and End-Of-Line */
    z_chunkq_remove(&(client->rdbuffer), n);
    if ((index = z_chunkq_indexof(&(client->rdbuffer), 0, "\n", 1)) >= 0)
        z_chunkq_remove(&(client->rdbuffer), index + 1);
}

static int __rpc_write_fserrno (z_rpc_client_t *client,
                                raleighfs_errno_t errno)
{
    const char *errmsg;

    if (errno == RALEIGHFS_ERRNO_NONE)
        return(z_rpc_write(client, "+OK\r\n", 5));

    if ((errmsg = raleighfs_errno_string(errno)) == NULL)
        errmsg = "unknown";

    if (raleighfs_errno_is_info(errno))
        z_rpc_write(client, "+OK ", 4);
    else
        z_rpc_write(client, "-ERR ", 5);
    z_rpc_write(client, errmsg, z_strlen(errmsg));
    z_rpc_write(client, "\r\n", 2);
    return(0);
}

static int __rpc_send_error (z_rpc_client_t *client, rpc_errno_t errno) {
    switch (errno) {
        case RPC_ERRNO_GENERIC:
            return(z_rpc_write(client, "-ERR\r\n", 6));
        case RPC_ERRNO_BAD_LINE:
            return(z_rpc_write(client, "-ERR bad line\r\n", 15));
        case RPC_ERRNO_INVALID_COMMAND:
            return(z_rpc_write(client, "-ERR invalid command\r\n", 22));
    }
    return(-1);
}

/* ============================================================================
 *  Semantic Objects
 *   - create <name> <type>
 *   - delete <name>
 *   - exists <name>
 *   - move   <old name> <new name>
 */
static const uint8_t *__semantic_object_type (const z_chunkq_extent_t *name) {
    switch (name->length) {
        case 2:
            if (!__tokcmp(name, "kv", 2))
                return(RALEIGHFS_OBJECT_MEMCACHE_UUID);
            break;
        case 4:
            if (!__tokcmp(name, "sset", 4))
                return(RALEIGHFS_OBJECT_SSET_UUID);
            break;
        case 5:
            if (!__tokcmp(name, "deque", 5))
                return(RALEIGHFS_OBJECT_DEQUE_UUID);
            break;
        case 7:
            if (!__tokcmp(name, "counter", 7))
                return(RALEIGHFS_OBJECT_COUNTER_UUID);
            break;
    }

    return(NULL);
}

static void __complete_semantic_create (void *client, z_message_t *msg) {
    __rpc_write_fserrno(Z_RPC_CLIENT(client), z_message_state(msg));
    z_message_free(msg);
}

static int __rpc_semantic_create (z_rpc_client_t *client,
                                  unsigned int method_code,
                                  const z_chunkq_extent_t *tokens,
                                  unsigned int ntokens,
                                  unsigned int nline)
{
    const uint8_t *object_type;
    z_stream_t stream;
    z_message_t *msg;

    if (ntokens != 4)
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    if ((object_type = __semantic_object_type(&(tokens[ARG0_TOKEN]))) == NULL)
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    /* Send semantic create request */
    msg = __rpc_message_alloc(client, Z_MESSAGE_WRITE_REQUEST);
    z_message_set_type(msg, RALEIGHFS_CREATE);
    z_message_request_stream(msg, &stream);
    z_stream_write(&stream, object_type, 16);

    return(__rpc_message_send(client, tokens, msg, __complete_semantic_create));
}

static void __complete_semantic_unlink (void *client, z_message_t *msg) {
    __rpc_write_fserrno(Z_RPC_CLIENT(client), z_message_state(msg));
    z_message_free(msg);
}

static int __rpc_semantic_unlink (z_rpc_client_t *client,
                                  unsigned int method_code,
                                  const z_chunkq_extent_t *tokens,
                                  unsigned int ntokens,
                                  unsigned int nline)
{
    z_message_t *msg;

    /* Send semantic unlink request */
    msg = __rpc_message_alloc(client, Z_MESSAGE_WRITE_REQUEST);
    z_message_set_type(msg, RALEIGHFS_UNLINK);

    return(__rpc_message_send(client, tokens, msg, __complete_semantic_unlink));
}

static void __complete_semantic_exists (void *client, z_message_t *msg) {
    raleighfs_errno_t errno;

    switch ((errno = (z_message_state(msg) & ~ RALEIGHFS_ERRNO_INFO))) {
        case RALEIGHFS_ERRNO_OBJECT_EXISTS:
            z_rpc_write(Z_RPC_CLIENT(client), "+EXISTS\r\n", 9);
            break;
        case RALEIGHFS_ERRNO_OBJECT_NOT_FOUND:
            z_rpc_write(Z_RPC_CLIENT(client), "+NOT-FOUND\r\n", 12);
            break;
        default:
            __rpc_write_fserrno(Z_RPC_CLIENT(client), errno);
            break;
    }
    z_message_free(msg);
}

static int __rpc_semantic_exists (z_rpc_client_t *client,
                                  unsigned int method_code,
                                  const z_chunkq_extent_t *tokens,
                                  unsigned int ntokens,
                                  unsigned int nline)
{
    z_message_t *msg;

    /* Send semantic exists request */
    msg = __rpc_message_alloc(client, Z_MESSAGE_READ_REQUEST);
    z_message_set_type(msg, RALEIGHFS_EXISTS);

    return(__rpc_message_send(client, tokens, msg, __complete_semantic_exists));
}

static void __complete_semantic_rename (void *client, z_message_t *msg) {
    __rpc_write_fserrno(Z_RPC_CLIENT(client), z_message_state(msg));
    z_message_free(msg);
}

static int __rpc_semantic_rename (z_rpc_client_t *client,
                                  unsigned int method_code,
                                  const z_chunkq_extent_t *tokens,
                                  unsigned int ntokens,
                                  unsigned int nline)
{
    const z_chunkq_extent_t *nnew;
    z_stream_t stream;
    z_message_t *msg;

    nnew = &(tokens[ARG0_TOKEN]);

    /* Send Semantic Rename Request! */
    msg = __rpc_message_alloc(client, Z_MESSAGE_WRITE_REQUEST);
    z_message_set_type(msg, RALEIGHFS_RENAME);
    z_message_request_stream(msg, &stream);
    z_stream_write_uint32(&stream, nnew->length);
    z_stream_write_chunkq(&stream, nnew->chunkq, nnew->offset, nnew->length);

    return(__rpc_message_send(client, tokens, msg, __complete_semantic_rename));
}

/* ============================================================================
 *  Sorted Set Objects Commands
 */
static int __memcache_write_state (z_rpc_client_t *client,
                                   raleighfs_memcache_state_t state)
{
    unsigned int length;
    const char *message;

    switch (state) {
        case RALEIGHFS_MEMCACHE_BAD_CMDLINE:
            message = "-ERR CLIENT_ERROR bad command line format\r\n";
            length  = 43;
            break;
        case RALEIGHFS_MEMCACHE_OBJ_TOO_LARGE:
            message = "-ERR SERVER_ERROR object too large for cache\r\n";
            length  = 46;
            break;
        case RALEIGHFS_MEMCACHE_NO_MEMORY:
            message = "-ERR SERVER_ERROR out of memory storing object\r\n";
            length  = 48;
            break;
        case RALEIGHFS_MEMCACHE_INVALID_DELTA:
            message = "-ERR CLIENT_ERROR invalid numeric delta argument\r\n";
            length  = 50;
            break;
        case RALEIGHFS_MEMCACHE_NOT_NUMBER:
            message = "-ERR CLIENT_ERROR cannot increment or decrement non-numeric value\r\n";
            length  = 67;
            break;
        case RALEIGHFS_MEMCACHE_NOT_STORED:
            message = "-ERR NOT_STORED\r\n";
            length  = 17;
            break;
        case RALEIGHFS_MEMCACHE_EXISTS:
            message = "-ERR EXISTS\r\n";
            length = 13;
            break;
        case RALEIGHFS_MEMCACHE_OK:
            message = "+OK\r\n";
            length = 5;
            break;
        case RALEIGHFS_MEMCACHE_STORED:
            message = "+OK STORED\r\n";
            length = 12;
            break;
        case RALEIGHFS_MEMCACHE_DELETED:
            message = "+OK DELETED\r\n";
            length = 13;
            break;
        case RALEIGHFS_MEMCACHE_NOT_FOUND:
            message = "-ERR NOT_FOUND\r\n";
            length = 16;
            break;
        default:
            message = "-ERR unknown\r\n";
            length  = 14;
            break;
    }

    return(z_rpc_write(client, message, length));
}

static void __complete_kv_get (void *user_data, z_message_t *msg) {
    z_rpc_client_t *client = Z_RPC_CLIENT(user_data);
    z_stream_extent_t value;
    z_stream_extent_t key;
    z_stream_t req_stream;
    z_stream_t res_stream;
    char text_number[16];
    char buffer[256];
    uint32_t klength;
    uint32_t vlength;
    uint32_t exptime;
    uint64_t number;
    uint32_t count;
    uint32_t state;
    uint32_t flags;
    uint64_t cas;
    int n;

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

            n = z_snprintf(buffer, sizeof(buffer), "+%u4d %u8d ", vlength, cas);
            z_rpc_write(client, buffer, n);
            z_rpc_write_stream(client, &key);
            z_rpc_write_newline(client);
            z_rpc_write(client, text_number, vlength);
        } else {
            z_stream_read_uint32(&res_stream, &vlength);
            z_stream_set_extent(&res_stream, &value, res_stream.offset, vlength);
            z_stream_seek(&res_stream, res_stream.offset + vlength);

            n = z_snprintf(buffer, sizeof(buffer), "+%u4d %u8d ", vlength, cas);
            z_rpc_write(client, buffer, n);
            z_rpc_write_stream(client, &key);
            z_rpc_write_newline(client);
            z_rpc_write_stream(client, &value);
        }

        z_rpc_write_newline(client);
    }

    z_rpc_write(client, "END\r\n", 5);

    z_message_free(msg);
}

static int __rpc_kv_get (z_rpc_client_t *client,
                         unsigned int method_code,
                         const z_chunkq_extent_t *tokens,
                         unsigned int ntokens,
                         unsigned int nline)
{
    const z_chunkq_extent_t *key;
    z_stream_t stream;
    z_message_t *msg;
    unsigned int i;

    if (ntokens < 4)
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    for (i = ARG0_TOKEN; i < ntokens; ++i) {
        if (tokens[i].length > KEY_MAX_LENGTH)
            return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));
    }

    /* Send GET Operation Command! */
    msg = __rpc_message_alloc(client, Z_MESSAGE_READ_REQUEST);
    z_message_set_type(msg, method_code);
    z_message_request_stream(msg, &stream);
    z_stream_write_uint32(&stream, ntokens - ARG0_TOKEN);

    for (i = ARG0_TOKEN; i < ntokens; ++i) {
        key = &(tokens[i]);
        z_stream_write_uint32(&stream, key->length);
        z_stream_write_chunkq(&stream, key->chunkq, key->offset, key->length);
    }

    return(__rpc_message_send(client, tokens, msg, __complete_kv_get));
}

static void __complete_kv_update (void *client, z_message_t *msg) {
    __memcache_write_state(Z_RPC_CLIENT(client), z_message_state(msg));
    z_message_free(msg);
}

static int __rpc_kv_update (z_rpc_client_t *client,
                            unsigned int code,
                            const z_chunkq_extent_t *tokens,
                            unsigned int ntokens,
                            unsigned int nline)
{
    const z_chunkq_extent_t *key;
    uint64_t req_cas_id = 0;
    z_message_t *msg;
    z_stream_t stream;
    uint32_t vlength;
    uint32_t exptime = 0;
    uint32_t flags = 0;
    int res;

    /* kv set /object <key> <length> <cas> */
    if (ntokens < 4 || ntokens > 6)
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    key = &(tokens[ARG0_TOKEN]);
    if (key->length > KEY_MAX_LENGTH)
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    if (!__tok2u32(&(tokens[ARG1_TOKEN]), &vlength))
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    /* Data is not available yet */
    if (client->rdbuffer.size < (nline + vlength))
        return(1);

    if (code == RALEIGHFS_MEMCACHE_CAS && !__tok2u64(&(tokens[ARG2_TOKEN]), &req_cas_id))
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    /* Send SET Operation Command! */
    msg = __rpc_message_alloc(client, Z_MESSAGE_WRITE_REQUEST);
    z_message_set_type(msg, code);
    z_message_request_stream(msg, &stream);
    z_stream_write_uint32(&stream, key->length);
    z_stream_write_uint32(&stream, vlength);
    z_stream_write_uint32(&stream, flags);
    z_stream_write_uint32(&stream, exptime);
    z_stream_write_uint64(&stream, req_cas_id);
    z_stream_write_chunkq(&stream, key->chunkq, key->offset, key->length);
    z_stream_write_chunkq(&stream, &(client->rdbuffer), nline, vlength);
    res = __rpc_message_send(client, tokens, msg, __complete_kv_update);

    /* Remove request */
    __rpc_client_remove_to_eol(client, nline + vlength);

    return(res);
}


static void __complete_kv_remove (void *client, z_message_t *msg) {
    __memcache_write_state(Z_RPC_CLIENT(client), z_message_state(msg));
    z_message_free(msg);
}

static int __rpc_kv_remove (z_rpc_client_t *client,
                            unsigned int method_code,
                            const z_chunkq_extent_t *tokens,
                            unsigned int ntokens,
                            unsigned int nline)
{
    const z_chunkq_extent_t *key;
    z_message_t *msg;
    z_stream_t stream;

    if (ntokens != 4)
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    key = &(tokens[ARG0_TOKEN]);

    /* Send Delete Operation Command! */
    msg = __rpc_message_alloc(client, Z_MESSAGE_WRITE_REQUEST);
    z_message_set_type(msg, method_code);
    z_message_request_stream(msg, &stream);
    z_stream_write_uint32(&stream, key->length);
    z_stream_write_chunkq(&stream, key->chunkq, key->offset, key->length);
    return(__rpc_message_send(client, tokens, msg, __complete_kv_remove));
}

static void __complete_kv_clear (void *client, z_message_t *msg) {
    __memcache_write_state(Z_RPC_CLIENT(client), z_message_state(msg));
    z_message_free(msg);
}

static int __rpc_kv_clear (z_rpc_client_t *client,
                           unsigned int method_code,
                           const z_chunkq_extent_t *tokens,
                           unsigned int ntokens,
                           unsigned int nline)
{
    z_message_t *msg;

    if (ntokens != 3)
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    /* Send Flush Operation Command! */
    msg = __rpc_message_alloc(client, Z_MESSAGE_WRITE_REQUEST);
    z_message_set_type(msg, method_code);
    return(__rpc_message_send(client, tokens, msg, __complete_kv_clear));
}

/* ============================================================================
 *  Sorted Set Objects Commands
 */
static void __complete_sset_update (void *client, z_message_t *msg) {
    __rpc_write_fserrno(Z_RPC_CLIENT(client), z_message_state(msg));
}

static int __rpc_sset_update (z_rpc_client_t *client,
                              unsigned int method_code,
                              const z_chunkq_extent_t *tokens,
                              unsigned int ntokens,
                              unsigned int nline)
{
    const z_chunkq_extent_t *key;
    z_stream_t stream;
    z_message_t *msg;
    uint32_t vlength;
    int res;

    if (ntokens != 5)
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    key = &(tokens[ARG0_TOKEN]);
    if (!__tok2u32(&(tokens[ARG1_TOKEN]), &vlength))
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    /* Data is not available yet */
    if (client->rdbuffer.size < (nline + vlength))
        return(1);

    msg = __rpc_message_alloc(client, Z_MESSAGE_WRITE_REQUEST);
    z_message_set_type(msg, method_code);
    z_message_request_stream(msg, &stream);
    z_stream_write_uint32(&stream, key->length);
    z_stream_write_uint32(&stream, vlength);
    z_stream_write_chunkq(&stream, key->chunkq, key->offset, key->length);
    z_stream_write_chunkq(&stream, &(client->rdbuffer), nline, vlength);
    res = __rpc_message_send(client, tokens, msg, __complete_sset_update);

    /* Remove request */
    __rpc_client_remove_to_eol(client, nline + vlength);

    return(res);
}

static void __complete_sset_get (void *client, z_message_t *msg) {
    if (z_message_state(msg) != RALEIGHFS_ERRNO_NONE) {
        __rpc_write_fserrno(Z_RPC_CLIENT(client), z_message_state(msg));
    } else {
        z_stream_extent_t value;
        z_stream_extent_t key;
        z_stream_t stream;
        uint32_t klength;
        uint32_t vlength;
        char buffer[16];
        int n;

        z_message_response_stream(msg, &stream);

        while (1) {
            z_stream_read_uint32(&stream, &klength);
            if (!klength) break;

            z_stream_read_uint32(&stream, &vlength);
            z_stream_set_extent(&stream, &key, stream.offset, klength);
            z_stream_set_extent(&stream, &value, stream.offset + klength, vlength);
            z_stream_seek(&stream, value.offset + value.length);

            n = z_snprintf(buffer, sizeof(buffer), "+%u4d ", vlength);
            z_rpc_write(Z_RPC_CLIENT(client), buffer, n);
            z_rpc_write_stream(Z_RPC_CLIENT(client), &key);
            z_rpc_write_newline(Z_RPC_CLIENT(client));

            if (vlength > 0) {
                z_rpc_write_stream(Z_RPC_CLIENT(client), &value);
                z_rpc_write_newline(Z_RPC_CLIENT(client));
            }
        }

        z_rpc_write(Z_RPC_CLIENT(client), "END\r\n", 5);
    }
    z_message_free(msg);
}


static void __complete_sset_remove (void *client, z_message_t *msg) {
    __rpc_write_fserrno(Z_RPC_CLIENT(client), z_message_state(msg));
}

static int __rpc_sset_range (z_rpc_client_t *client,
                             unsigned int method_code,
                             const z_chunkq_extent_t *tokens,
                             unsigned int ntokens,
                             unsigned int nline)
{
    const z_chunkq_extent_t *a;
    const z_chunkq_extent_t *b;
    z_message_func_t callback;
    unsigned int msg_flags;
    z_message_t *msg;
    z_stream_t stream;

    if (ntokens != 5)
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    switch (method_code) {
        case RALEIGHFS_SSET_POP_RANGE:
        case RALEIGHFS_SSET_GET_RANGE:
            callback = __complete_sset_get;
            msg_flags = Z_MESSAGE_READ_REQUEST;
            break;
        case RALEIGHFS_SSET_RM_RANGE:
            callback = __complete_sset_remove;
            msg_flags = Z_MESSAGE_WRITE_REQUEST;
            break;
        default:
            /* Never reached! */
            break;
    }

    a = &(tokens[ARG0_TOKEN]);
    b = &(tokens[ARG1_TOKEN]);

    msg = __rpc_message_alloc(client, msg_flags);
    z_message_set_type(msg, method_code);
    z_message_request_stream(msg, &stream);
    z_stream_write_uint32(&stream, a->length);
    z_stream_write_uint32(&stream, b->length);
    z_stream_write_chunkq(&stream, a->chunkq, a->offset, a->length);
    z_stream_write_chunkq(&stream, b->chunkq, b->offset, b->length);

    return(__rpc_message_send(client, tokens, msg, callback));
}

static int __rpc_sset_index (z_rpc_client_t *client,
                             unsigned int method_code,
                             const z_chunkq_extent_t *tokens,
                             unsigned int ntokens,
                             unsigned int nline)
{
    z_message_func_t callback;
    unsigned int msg_flags;
    z_stream_t stream;
    z_message_t *msg;
    uint64_t a, b;

    if (ntokens != 5)
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    if (!__tok2u64(&(tokens[ARG0_TOKEN]), &a))
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    if (!__tok2u64(&(tokens[ARG1_TOKEN]), &b))
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    switch (method_code) {
        case RALEIGHFS_SSET_POP_INDEX:
        case RALEIGHFS_SSET_GET_INDEX:
            callback = __complete_sset_get;
            msg_flags = Z_MESSAGE_READ_REQUEST;
            break;
        case RALEIGHFS_SSET_RM_INDEX:
            callback = __complete_sset_remove;
            msg_flags = Z_MESSAGE_WRITE_REQUEST;
            break;
        default:
            /* Never reached! */
            break;
    }

    msg = __rpc_message_alloc(client, msg_flags);
    z_message_set_type(msg, method_code);
    z_message_request_stream(msg, &stream);
    z_stream_write_uint64(&stream, a);
    z_stream_write_uint64(&stream, b);

    return(__rpc_message_send(client, tokens, msg, callback));
}

static int __rpc_sset_items (z_rpc_client_t *client,
                             unsigned int method_code,
                             const z_chunkq_extent_t *tokens,
                             unsigned int ntokens,
                             unsigned int nline)
{
    const z_chunkq_extent_t *tk;
    z_message_func_t callback;
    unsigned int msg_flags;
    z_stream_t stream;
    z_message_t *msg;
    unsigned int i;

    switch (method_code) {
        case RALEIGHFS_SSET_RM:
        case RALEIGHFS_SSET_GET:
        case RALEIGHFS_SSET_POP:
            if (ntokens > 4)
                return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));
            break;
        default:
            if (ntokens != 3)
                return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));
            break;
    }

    switch (method_code) {
        case RALEIGHFS_SSET_POP:
        case RALEIGHFS_SSET_GET:
        case RALEIGHFS_SSET_POP_LAST:
        case RALEIGHFS_SSET_GET_LAST:
        case RALEIGHFS_SSET_POP_FIRST:
        case RALEIGHFS_SSET_GET_FIRST:
            callback = __complete_sset_get;
            msg_flags = Z_MESSAGE_READ_REQUEST;
            break;
        case RALEIGHFS_SSET_RM:
        case RALEIGHFS_SSET_RM_LAST:
        case RALEIGHFS_SSET_RM_FIRST:
            callback = __complete_sset_remove;
            msg_flags = Z_MESSAGE_WRITE_REQUEST;
            break;
        default:
            /* Never reached! */
            break;
    }

    msg = __rpc_message_alloc(client, msg_flags);
    z_message_set_type(msg, method_code);
    z_message_request_stream(msg, &stream);

    if ((ntokens - ARG0_TOKEN) > 0) {
        z_stream_write_uint32(&stream, ntokens - ARG0_TOKEN);

        for (i = ARG0_TOKEN; i < ntokens; ++i) {
            tk = &(tokens[i]);
            z_stream_write_uint32(&stream, tk->length);
            z_stream_write_chunkq(&stream, tk->chunkq, tk->offset, tk->length);
        }
    }

    return(__rpc_message_send(client, tokens, msg, callback));
}

static void __complete_sset_keys (void *client, z_message_t *msg) {
    if (z_message_state(msg) != RALEIGHFS_ERRNO_NONE) {
        __rpc_write_fserrno(Z_RPC_CLIENT(client), z_message_state(msg));
    } else {
        z_stream_extent_t key;
        z_stream_t stream;
        uint32_t klength;
        char buffer[16];
        uint64_t count;
        int n;

        z_message_response_stream(msg, &stream);
        z_stream_read_uint64(&stream, &count);

        n = z_snprintf(buffer, sizeof(buffer), "+%u8d\r\n", count);
        z_rpc_write(Z_RPC_CLIENT(client), buffer, n);

        while (count--) {
            z_stream_read_uint32(&stream, &klength);
            z_stream_set_extent(&stream, &key, stream.offset, klength);
            z_stream_seek(&stream, key.offset + key.length);

            z_rpc_write_stream(Z_RPC_CLIENT(client), &key);
            z_rpc_write_newline(Z_RPC_CLIENT(client));
        }
    }
    z_message_free(msg);
}

static int __rpc_sset_keys (z_rpc_client_t *client,
                            unsigned int method_code,
                            const z_chunkq_extent_t *tokens,
                            unsigned int ntokens,
                            unsigned int nline)
{
    z_message_t *msg;

    if (ntokens != 3)
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    msg = __rpc_message_alloc(client, Z_MESSAGE_READ_REQUEST);
    z_message_set_type(msg, method_code);

    return(__rpc_message_send(client, tokens, msg, __complete_sset_keys));
}

static void __complete_sset_wquery (void *client, z_message_t *msg) {
    __rpc_write_fserrno(Z_RPC_CLIENT(client), z_message_state(msg));
}

static int __rpc_sset_wquery (z_rpc_client_t *client,
                              unsigned int method_code,
                              const z_chunkq_extent_t *tokens,
                              unsigned int ntokens,
                              unsigned int nline)
{
    z_message_t *msg;

    if (ntokens != 3)
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    msg = __rpc_message_alloc(client, Z_MESSAGE_WRITE_REQUEST);
    z_message_set_type(msg, method_code);

    return(__rpc_message_send(client, tokens, msg, __complete_sset_wquery));
}

static void __complete_sset_length (void *client, z_message_t *msg) {
    if (z_message_state(msg) != RALEIGHFS_ERRNO_NONE) {
        __rpc_write_fserrno(Z_RPC_CLIENT(client), z_message_state(msg));
    } else {
        z_stream_t stream;
        uint64_t length;
        char buffer[32];
        int n;

        z_message_response_stream(msg, &stream);
        z_stream_read_uint64(&stream, &length);

        n = z_snprintf(buffer, sizeof(buffer), "+%u8d\r\n", length);
        z_rpc_write(Z_RPC_CLIENT(client), buffer, n);
    }
    z_message_free(msg);
}

static int __rpc_sset_length (z_rpc_client_t *client,
                               unsigned int method_code,
                               const z_chunkq_extent_t *tokens,
                               unsigned int ntokens,
                               unsigned int nline)
{
    z_message_t *msg;

    if (ntokens != 3)
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    msg = __rpc_message_alloc(client, Z_MESSAGE_READ_REQUEST);
    z_message_set_type(msg, method_code);

    return(__rpc_message_send(client, tokens, msg, __complete_sset_length));
}

static void __complete_sset_stats (void *client, z_message_t *msg) {
    if (z_message_state(msg) != RALEIGHFS_ERRNO_NONE) {
        __rpc_write_fserrno(Z_RPC_CLIENT(client), z_message_state(msg));
    } else {
        char buffer[32];
        int n;

        /* TODO: Key/Value stats */
        n = z_snprintf(buffer, sizeof(buffer), "+TODO\r\n");
        z_rpc_write(Z_RPC_CLIENT(client), buffer, n);
    }
    z_message_free(msg);
}

static int __rpc_sset_stats (z_rpc_client_t *client,
                             unsigned int method_code,
                             const z_chunkq_extent_t *tokens,
                             unsigned int ntokens,
                             unsigned int nline)
{
    z_message_t *msg;

    if (ntokens != 3)
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    msg = __rpc_message_alloc(client, Z_MESSAGE_READ_REQUEST);
    z_message_set_type(msg, method_code);

    return(__rpc_message_send(client, tokens, msg, __complete_sset_stats));
}

/* ============================================================================
 *  Deque Object Commands
 */
static void __complete_deque_push (void *client, z_message_t *msg) {
    __rpc_write_fserrno(Z_RPC_CLIENT(client), z_message_state(msg));
    z_message_free(msg);
}

static int __rpc_deque_push (z_rpc_client_t *client,
                             unsigned int method_code,
                             const z_chunkq_extent_t *tokens,
                             unsigned int ntokens,
                             unsigned int nline)
{
    z_stream_t stream;
    z_message_t *msg;
    uint32_t vlength;
    int res;

    if (ntokens != 4)
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    if (!__tok2u32(&(tokens[ARG0_TOKEN]), &vlength))
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    /* Data is not available yet */
    if (client->rdbuffer.size < (nline + vlength))
        return(1);

    msg = __rpc_message_alloc(client, Z_MESSAGE_WRITE_REQUEST);
    z_message_set_type(msg, method_code);
    z_message_request_stream(msg, &stream);
    z_stream_write_uint32(&stream, vlength);
    z_stream_write_chunkq(&stream, &(client->rdbuffer), nline, vlength);
    res = __rpc_message_send(client, tokens, msg, __complete_deque_push);

    /* Remove request */
    __rpc_client_remove_to_eol(client, nline + vlength);

    return(res);
}

static void __complete_deque_get (void *client, z_message_t *msg) {
    if (z_message_state(msg) != RALEIGHFS_ERRNO_NONE) {
        __rpc_write_fserrno(Z_RPC_CLIENT(client), z_message_state(msg));
    } else {
        z_stream_extent_t value;
        z_stream_t stream;
        uint32_t vlength;
        char buffer[32];
        int n;

        z_message_response_stream(msg, &stream);
        z_stream_read_uint32(&stream, &vlength);

        n = z_snprintf(buffer, sizeof(buffer), "+%u4d\r\n", vlength);
        z_rpc_write(Z_RPC_CLIENT(client), buffer, n);

        if (vlength > 0) {
            z_stream_set_extent(&stream, &value, 4, vlength);
            z_rpc_write_stream(Z_RPC_CLIENT(client), &value);
            z_rpc_write(Z_RPC_CLIENT(client), "\r\n", 2);
        }
    }
    z_message_free(msg);
}

static int __rpc_deque_get (z_rpc_client_t *client,
                            unsigned int method_code,
                            const z_chunkq_extent_t *tokens,
                            unsigned int ntokens,
                            unsigned int nline)
{
    z_message_t *msg;

    if (ntokens != 3)
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    msg = __rpc_message_alloc(client, Z_MESSAGE_READ_REQUEST);
    z_message_set_type(msg, method_code);

    return(__rpc_message_send(client, tokens, msg, __complete_deque_get));
}

static void __complete_deque_remove (void *client, z_message_t *msg) {
    __rpc_write_fserrno(Z_RPC_CLIENT(client), z_message_state(msg));
    z_message_free(msg);
}

static int __rpc_deque_remove (z_rpc_client_t *client,
                               unsigned int method_code,
                               const z_chunkq_extent_t *tokens,
                               unsigned int ntokens,
                               unsigned int nline)
{
    z_message_t *msg;

    if (ntokens != 3)
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    msg = __rpc_message_alloc(client, Z_MESSAGE_WRITE_REQUEST);
    z_message_set_type(msg, method_code);

    return(__rpc_message_send(client, tokens, msg, __complete_deque_remove));
}

static void __complete_deque_length (void *client, z_message_t *msg) {
    if (z_message_state(msg) != RALEIGHFS_ERRNO_NONE) {
        __rpc_write_fserrno(Z_RPC_CLIENT(client), z_message_state(msg));
    } else {
        z_stream_t stream;
        uint64_t length;
        char buffer[32];
        int n;

        z_message_response_stream(msg, &stream);
        z_stream_read_uint64(&stream, &length);

        n = z_snprintf(buffer, sizeof(buffer), "+%u8d\r\n", length);
        z_rpc_write(Z_RPC_CLIENT(client), buffer, n);
    }
    z_message_free(msg);
}

static int __rpc_deque_length (z_rpc_client_t *client,
                               unsigned int method_code,
                               const z_chunkq_extent_t *tokens,
                               unsigned int ntokens,
                               unsigned int nline)
{
    z_message_t *msg;

    if (ntokens != 3)
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    msg = __rpc_message_alloc(client, Z_MESSAGE_READ_REQUEST);
    z_message_set_type(msg, method_code);

    return(__rpc_message_send(client, tokens, msg, __complete_deque_length));
}

static void __complete_deque_stats (void *client, z_message_t *msg) {
    if (z_message_state(msg) != RALEIGHFS_ERRNO_NONE) {
        __rpc_write_fserrno(Z_RPC_CLIENT(client), z_message_state(msg));
    } else {
        char buffer[32];
        int n;

        /* TODO: Key/Value stats */
        n = z_snprintf(buffer, sizeof(buffer), "+TODO\r\n");
        z_rpc_write(Z_RPC_CLIENT(client), buffer, n);
    }
}

static int __rpc_deque_stats (z_rpc_client_t *client,
                              unsigned int method_code,
                              const z_chunkq_extent_t *tokens,
                              unsigned int ntokens,
                              unsigned int nline)
{
    z_message_t *msg;

    if (ntokens != 3)
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    msg = __rpc_message_alloc(client, Z_MESSAGE_READ_REQUEST);
    z_message_set_type(msg, method_code);

    return(__rpc_message_send(client, tokens, msg, __complete_deque_stats));
}

/* ============================================================================
 *  Counter Object Commands
 *      - set <value>
 *      - cas <value> <cas>
 *      - incr [value]
 *      - decr [value]
 */
static void __complete_counter_math (void *client, z_message_t *msg) {
    z_stream_t stream;
    char buffer[64];
    uint64_t value;
    uint64_t cas;
    int n;

    if (z_message_state(msg) != RALEIGHFS_ERRNO_NONE) {
        __rpc_write_fserrno(Z_RPC_CLIENT(client), z_message_state(msg));
    } else {
        z_message_response_stream(msg, &stream);
        z_stream_read_uint64(&stream, &value);
        z_stream_read_uint64(&stream, &cas);

        n = z_snprintf(buffer, sizeof(buffer), "+%u8d %u8d\r\n", value, cas);
        z_rpc_write(Z_RPC_CLIENT(client), buffer, n);
    }

    z_message_free(msg);
}

static int __rpc_counter_get (z_rpc_client_t *client,
                              unsigned int method_code,
                              const z_chunkq_extent_t *tokens,
                              unsigned int ntokens,
                              unsigned int nline)
{
    z_message_t *msg;

    if (ntokens != 3)
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    msg = __rpc_message_alloc(client, Z_MESSAGE_READ_REQUEST);
    z_message_set_type(msg, method_code);

    return(__rpc_message_send(client, tokens, msg, __complete_counter_math));
}

static int __rpc_counter_update (z_rpc_client_t *client,
                                 unsigned int method_code,
                                 const z_chunkq_extent_t *tokens,
                                 unsigned int ntokens,
                                 unsigned int nline)
{
    z_stream_t stream;
    z_message_t *msg;
    uint64_t value;
    uint64_t cas;

    if (ntokens < 3 || ntokens > 5)
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    value = 1;
    if (ntokens >= 4 && !__tok2u64(&(tokens[ARG0_TOKEN]), &value))
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    if (ntokens == 5 && !__tok2u64(&(tokens[ARG1_TOKEN]), &cas))
        return(__rpc_send_error(client, RPC_ERRNO_BAD_LINE));

    msg = __rpc_message_alloc(client, Z_MESSAGE_WRITE_REQUEST);
    z_message_set_type(msg, method_code);
    z_message_request_stream(msg, &stream);
    z_stream_write_uint64(&stream, value);
    if (ntokens == 5)
        z_stream_write_uint64(&stream, cas);

    return(__rpc_message_send(client, tokens, msg, __complete_counter_math));
}

/* ============================================================================
 *  Server Commands
 */
static int __rpc_server_ping (z_rpc_client_t *client,
                              unsigned int method_code,
                              const z_chunkq_extent_t *tokens,
                              unsigned int ntokens,
                              unsigned int nline)
{
    return(z_rpc_write(client, "+PONG\r\n", 7));
}

static int __rpc_server_quit (z_rpc_client_t *client,
                              unsigned int method_code,
                              const z_chunkq_extent_t *tokens,
                              unsigned int ntokens,
                              unsigned int nline)
{
    z_rpc_write(client, "+BYE\r\n", 6);
    return(-1);
}

/* ============================================================================
 *  RPC Commands
 */

/* Key-Value Object */
static const struct rpc_command __kv_commands[] = {
    { "get",     3, RALEIGHFS_MEMCACHE_GETS,      1, __rpc_kv_get,    0 },
    { "set",     3, RALEIGHFS_MEMCACHE_SET,       0, __rpc_kv_update, 0 },
    { "cas",     3, RALEIGHFS_MEMCACHE_CAS,       0, __rpc_kv_update, 0 },
    { "add",     3, RALEIGHFS_MEMCACHE_ADD,       0, __rpc_kv_update, 0 },
    { "replace", 7, RALEIGHFS_MEMCACHE_REPLACE,   0, __rpc_kv_update, 0 },
    { "prepend", 7, RALEIGHFS_MEMCACHE_PREPEND,   0, __rpc_kv_update, 0 },
    { "append",  6, RALEIGHFS_MEMCACHE_APPEND,    0, __rpc_kv_update, 0 },
    { "remove",  6, RALEIGHFS_MEMCACHE_DELETE,    1, __rpc_kv_remove, 0 },
    { "clear",   9, RALEIGHFS_MEMCACHE_FLUSH_ALL, 1, __rpc_kv_clear,  0 },
    { NULL,      0, 0, 0, NULL, 0 },
};

/* Sorted Set Object */
static const struct rpc_command __sset_commands[] = {
    { "insert",      6, RALEIGHFS_SSET_INSERT,     0, __rpc_sset_update,  0 },
    { "update",      6, RALEIGHFS_SSET_UPDATE,     0, __rpc_sset_update,  0 },
    { "pop",         3, RALEIGHFS_SSET_POP,        1, __rpc_sset_items,   4 },
        { "-range",  6, RALEIGHFS_SSET_POP_RANGE,  1, __rpc_sset_range,   0 },
        { "-index",  6, RALEIGHFS_SSET_POP_INDEX,  1, __rpc_sset_index,   0 },
        { "-first",  6, RALEIGHFS_SSET_POP_FIRST,  1, __rpc_sset_items,   0 },
        { "-last",   5, RALEIGHFS_SSET_POP_LAST,   1, __rpc_sset_items,   0 },
    { "get",         3, RALEIGHFS_SSET_GET,        1, __rpc_sset_items,   4 },
        { "-range",  6, RALEIGHFS_SSET_GET_RANGE,  1, __rpc_sset_range,   0 },
        { "-index",  6, RALEIGHFS_SSET_GET_INDEX,  1, __rpc_sset_index,   0 },
        { "-first",  6, RALEIGHFS_SSET_GET_FIRST,  1, __rpc_sset_items,   0 },
        { "-last",   5, RALEIGHFS_SSET_GET_LAST,   1, __rpc_sset_items,   0 },
    { "rm",          2, RALEIGHFS_SSET_RM,         1, __rpc_sset_items,   4 },
        { "-range",  6, RALEIGHFS_SSET_RM_RANGE,   1, __rpc_sset_range,   0 },
        { "-index",  6, RALEIGHFS_SSET_RM_INDEX,   1, __rpc_sset_index,   0 },
        { "-first",  6, RALEIGHFS_SSET_RM_FIRST,   1, __rpc_sset_items,   0 },
        { "-last",   5, RALEIGHFS_SSET_RM_LAST,    1, __rpc_sset_items,   0 },
    { "keys",        4, RALEIGHFS_SSET_GET_KEYS,   1, __rpc_sset_keys,    0 },
    { "clear",       5, RALEIGHFS_SSET_CLEAR,      1, __rpc_sset_wquery,  0 },
    { "reverse",     7, RALEIGHFS_SSET_REVERSE,    1, __rpc_sset_wquery,  0 },
    { "length",      6, RALEIGHFS_SSET_LENGTH,     1, __rpc_sset_length,  0 },
    { "stats",       5, RALEIGHFS_SSET_STATS,      1, __rpc_sset_stats,   0 },
    { NULL,          0, 0, 0, NULL, 0 },
};

/* Deque Object */
static const struct rpc_command __deque_commands[] = {
    { "push",       4, RALEIGHFS_DEQUE_PUSH_BACK,  0, __rpc_deque_push,   2 },
        { "-back",  5, RALEIGHFS_DEQUE_PUSH_BACK,  0, __rpc_deque_push,   0 },
        { "-front", 6, RALEIGHFS_DEQUE_PUSH_FRONT, 0, __rpc_deque_push,   0 },
    { "pop",        3, RALEIGHFS_DEQUE_POP_FRONT,  1, __rpc_deque_get,    2 },
        { "-back",  5, RALEIGHFS_DEQUE_POP_BACK,   1, __rpc_deque_get,    0 },
        { "-front", 6, RALEIGHFS_DEQUE_POP_FRONT,  1, __rpc_deque_get,    0 },
    { "get",        3, RALEIGHFS_DEQUE_GET_FRONT,  1, __rpc_deque_get,    2 },
        { "-back",  5, RALEIGHFS_DEQUE_GET_BACK,   1, __rpc_deque_get,    0 },
        { "-front", 6, RALEIGHFS_DEQUE_GET_FRONT,  1, __rpc_deque_get,    0 },
    { "rm",         2, RALEIGHFS_DEQUE_RM_FRONT,   1, __rpc_deque_remove, 2 },
        { "-back",  5, RALEIGHFS_DEQUE_RM_BACK,    1, __rpc_deque_remove, 0 },
        { "-front", 6, RALEIGHFS_DEQUE_RM_FRONT,   1, __rpc_deque_remove, 0 },
    { "clear",      5, RALEIGHFS_DEQUE_CLEAR,      1, __rpc_deque_remove, 0 },
    { "length",     6, RALEIGHFS_DEQUE_LENGTH,     1, __rpc_deque_length, 0 },
    { "stats",      5, RALEIGHFS_DEQUE_STATS,      1, __rpc_deque_stats,  0 },
    { NULL,         0, 0, 0, NULL, 0 },
};

/* Counter Object */
static const struct rpc_command __counter_commands[] = {
    { "get",   3, RALEIGHFS_COUNTER_GET,  1, __rpc_counter_get,    0 },
    { "incr",  4, RALEIGHFS_COUNTER_INCR, 1, __rpc_counter_update, 0 },
    { "decr",  4, RALEIGHFS_COUNTER_DECR, 1, __rpc_counter_update, 0 },
    { "set",   3, RALEIGHFS_COUNTER_SET,  1, __rpc_counter_update, 0 },
    { "cas",   3, RALEIGHFS_COUNTER_CAS,  1, __rpc_counter_update, 0 },
    { NULL,    0, 0, 0, NULL, 0 },
};

/* Semantic */
static const struct rpc_command __semantic_commands[] = {
    { "create", 6, 0, 1, __rpc_semantic_create, 0 },
    { "unlink", 6, 0, 1, __rpc_semantic_unlink, 0 },
    { "exists", 6, 0, 1, __rpc_semantic_exists, 0 },
    { "rename", 6, 0, 1, __rpc_semantic_rename, 0 },
    { NULL,     0, 0, 0, NULL, 0 },
};

/* Server */
static const struct rpc_command __server_commands[] = {
    { "ping",   4, 0, 1, __rpc_server_ping, 0 },
    { "quit",   4, 0, 1, __rpc_server_quit, 0 },
    { NULL,     0, 0, 0, NULL, 0 },
};

static const struct rpc_command_group __rpc_command_group[] = {
    { "kv",       2, __kv_commands },
    { "sset",     4, __sset_commands },
    { "deque",    5, __deque_commands },
    { "counter",  7, __counter_commands },

    { "semantic", 8, __semantic_commands },
    { "server",   6, __server_commands },
    { NULL,       0, NULL },
};

/* ============================================================================
 *  V1 Protocol Methods
 */
static int __rpc_bind (const void *hostname, const void *service) {
    int socket;

    if ((socket = z_socket_tcp_bind(hostname, service, NULL)) < 0)
        return(-1);

    Z_PRINT_DEBUG(" - RaleighV1 Interface up and running on %s:%s...\n",
                  (const char *)hostname, (const char *)service);

    return(socket);
}

static int __rpc_accept (z_rpc_server_t *server) {
    struct sockaddr_storage address;
    char ip[INET6_ADDRSTRLEN];
    int csock;

    if ((csock = z_socket_tcp_accept(Z_IOPOLL_ENTITY_FD(server), NULL)) < 0)
        return(-1);

    z_socket_address(csock, &address);
    z_socket_str_address(ip, INET6_ADDRSTRLEN, (struct sockaddr *)&address);
    Z_PRINT_DEBUG(" - V1 Accept %d %s.\n", csock, ip);

    return(csock);
}

static int __rpc_connected (z_rpc_client_t *client) {
    z_message_source_t *source;

    if ((source = z_message_source_alloc((client)->server->user_data)) == NULL)
        return(1);

    client->user_data = source;
    return(0);
}

static void __rpc_disconnected (z_rpc_client_t *client) {
    z_message_source_free(client->user_data);
}

static int __rpc_process_cmd (z_rpc_client_t *client,
                              const struct rpc_command *command,
                              const z_chunkq_extent_t *tokens,
                              unsigned int ntokens,
                              unsigned int nline)
{
    int result;

    z_iopoll_entity_reset_flags(client);
    if ((result = command->func(client, command->code, tokens, ntokens, nline)))
        return(result);

    if (command->rmline)
        z_chunkq_remove(&(client->rdbuffer), nline);

    return(0);
}

static int __rpc_process_line (z_rpc_client_t *client, unsigned int n) {
    z_chunkq_extent_t tokens[MAX_TOKENS];
    const struct rpc_command_group *g;
    const struct rpc_command *p;
    const z_chunkq_extent_t *cmd;
    unsigned int ntokens;
    unsigned int i;

    ntokens = z_rpc_ntokenize(client, tokens, MAX_TOKENS, 0, n);
    cmd = &(tokens[COMMAND_TOKEN]);
    for (g = __rpc_command_group; g->commands != NULL; ++g) {
        if (g->length != tokens[NAMESPACE_TOKEN].length)
            continue;

        if (__tokcmp(&(tokens[NAMESPACE_TOKEN]), g->name, g->length))
            continue;

        for (p = g->commands; p->func != NULL; ++p) {
            if (p->subs == 0) {
                if (p->length != cmd->length)
                    continue;

                if (__tokcmp(cmd, p->name, p->length))
                    continue;

                return(__rpc_process_cmd(client, p, tokens, ntokens, n));
            } else {
                if (p->length > cmd->length) {
                    p += p->subs;
                    continue;
                }

                if (__tokcmp(cmd, p->name, p->length)) {
                    p += p->subs;
                    continue;
                }

                if (p->length == cmd->length)
                    return(__rpc_process_cmd(client, p, tokens, ntokens, n));

                for (i = 1; i <= p->subs; ++i) {
                    if ((p[i].length + p->length) != cmd->length)
                        continue;

                    if (__tokocmp(cmd, p->length, p[i].name, p[i].length))
                        continue;

                    return(__rpc_process_cmd(client, p+i, tokens, ntokens, n));
                }

                p += p->subs;
            }
        }
    }

    /* Invalid Command */
    z_chunkq_remove(&(client->rdbuffer), n);
    return(__rpc_send_error(client, RPC_ERRNO_INVALID_COMMAND));
}

z_rpc_protocol_t raleigh_v1_protocol = {
    .bind           = __rpc_bind,
    .accept         = __rpc_accept,
    .connected      = __rpc_connected,
    .disconnected   = __rpc_disconnected,
    .process        = NULL,
    .process_line   = __rpc_process_line,
};

