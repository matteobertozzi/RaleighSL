#include <zcl/coding.h>

#include <zcl/string.h>
#include <zcl/iobuf.h>
#include <zcl/ipc.h>

#include <unistd.h>
#include <stdio.h>

#include "rpc/generated/stats.h"
#include "rpc/generated/rpc.h"

#include "server.h"

static int __iobuf_write_buf (struct raleighfs_client *client,
                              const void *blob,
                              size_t size)
{
    const unsigned char *p = (const unsigned char *)blob;
    unsigned char *buf;
    long n, wn;

    while (size > 0) {
        wn = z_iobuf_write(&(client->obuffer), &buf); /* assert(n > 0) */
        n = z_min(wn, size);
        z_memcpy(buf, p, n);
        z_iobuf_write_backup(&(client->obuffer), wn - n);
        size -= n;
        p += n;
    }

    return((const void *)p - blob);
}

static void __msg_parse (struct raleighfs_client *client, const z_ipc_msg_reader_t *reader) {
    unsigned char buf[32];
    int elen;

    /* send response */
    elen = z_encode_vint(buf, 10);
    z_memcpy(buf + elen, "+ok012345!", 10);
    __iobuf_write_buf(client, buf, elen + 10);
    z_ipc_client_set_writable(client, 1);
}

static int __client_connected (z_ipc_client_t *ipc_client) {
    struct raleighfs_client *client = (struct raleighfs_client *)ipc_client;
    z_ipc_msgbuf_open(&(client->msgbuf), z_ipc_client_memory(client));
    z_iobuf_alloc(&(client->obuffer), z_ipc_client_memory(client), 128);
    client->field_id = 0;
    printf("RaleighFS client connected\n");
    return(0);
}

static void __client_disconnected (z_ipc_client_t *ipc_client) {
    struct raleighfs_client *client = (struct raleighfs_client *)ipc_client;
    printf("RaleighFS client disconnected\n");
    printf(" pre-close refs=%d\n", client->msgbuf.refs);
    z_ipc_msgbuf_close(&(client->msgbuf));
    printf("post-close refs=%d\n", client->msgbuf.refs);
    z_iobuf_free(&(client->obuffer));
}

static int __client_read (z_ipc_client_t *ipc_client) {
    struct raleighfs_client *client = (struct raleighfs_client *)ipc_client;
    z_ipc_msg_t msg;

    while (z_ipc_msgbuf_add(&(client->msgbuf), Z_IOPOLL_ENTITY_FD(client)) > 0) {
        while (z_ipc_msgbuf_get(&(client->msgbuf), &msg) != NULL) {
            struct rpc_reqhead reqhead;
            z_ipc_msg_reader_t reader;

            printf("msg_get offset=%3u length=%3lu ",
                    msg.offset, msg.length);
            //for (i = 0; i < msg.length; ++i) printf("=");
            printf("\n");

            /* Parse Request */
            z_reader_open(&reader, &msg);
            rpc_reqhead_parse(&reqhead, &reader);
            rpc_reqhead_dump(stdout, &reqhead);
            __msg_parse(client, &reader);
            z_reader_close(&reader);

            z_ipc_msg_free(&msg);
        }
    }

    return(0);
}

#if 0
    switch (request->type) {
        /* Server */
        case RALEIGH_SERVER_PING:
        case RALEIGH_SERVER_CASE:
        case RALEIGH_SERVER_STATS:
        /* Semantic */
        case RALEIGH_SEMANTIC_CREATE:
        case RALEIGH_SEMANTIC_DELETE:
        case RALEIGH_SEMANTIC_RENAME:
        case RALEIGH_SEMANTIC_EXISTS:
        /* Counter */
        case RALEIGH_COUNTER_GET:
        case RALEIGH_COUNTER_SET:
        case RALEIGH_COUNTER_CAS:
        case RALEIGH_COUNTER_INC:
        case RALEIGH_COUNTER_DEC:
        case RALEIGH_COUNTER_STATS:
    }
#endif

static int __client_write (z_ipc_client_t *ipc_client) {
    struct raleighfs_client *client = (struct raleighfs_client *)ipc_client;
    unsigned char *buffer;
    ssize_t n;

    while ((n = z_iobuf_read(&(client->obuffer), &buffer)) > 0) {
        ssize_t wr;
        if ((wr = write(Z_IOPOLL_ENTITY_FD(client), buffer, n)) < n) {
            z_iobuf_read_backup(&(client->obuffer), n - wr);
            break;
        }
    }

    z_ipc_client_set_writable(client, 0);
    return(0);
}

const struct z_ipc_protocol raleighfs_protocol = {
    /* server protocol */
    .bind         = z_ipc_bind_tcp,
    .accept       = z_ipc_accept_tcp,
    .setup        = NULL,

    /* client protocol */
    .connected    = __client_connected,
    .disconnected = __client_disconnected,
    .read         = __client_read,
    .write        = __client_write,
};
