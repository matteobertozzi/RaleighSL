#include <zcl/iopoll.h>
#include <zcl/chunkq.h>
#include <zcl/socket.h>
#include <zcl/base64.h>
#include <zcl/hash.h>
#include <zcl/rpc.h>

#include <unistd.h>
#include <ctype.h>
#include <stdio.h>

/*
 * Chrome 14
 * ============================================================================
 * TOKENIZE 0:18 'GET /echo HTTP/1.1'...
 * TOKENIZE 20:18 'Upgrade: websocket'...
 * TOKENIZE 40:19 'Connection: Upgrade'...
 * TOKENIZE 61:21 'Host: localhost:11217'...
 * TOKENIZE 84:26 'Sec-WebSocket-Origin: null'...
 * TOKENIZE 112:43 'Sec-WebSocket-Key: 91jGpG6JAG+pVEFE3H9LFA=='...
 * TOKENIZE 157:24 'Sec-WebSocket-Version: 8'...
 */

#define __key_cmp(client, token, name, len)                                   \
    z_chunkq_compare(&((client)->rdbuffer), (token)->offset, name, len,       \
                     (z_chunkq_compare_t)z_strncasecmp)

/* ============================================================================
 *  WebSocket Protocol Methods
 */
static int __websock_bind (const void *hostname, const void *service) {
    int socket;

    if ((socket = z_socket_tcp_bind(hostname, service, NULL)) < 0)
        return(-1);

    fprintf(stderr, " - WebSock Interface up and running on %s:%s...\n",
            (const char *)hostname, (const char *)service);

    return(socket);
}

static int __websock_accept (z_rpc_server_t *server) {
    struct sockaddr_storage address;
    char ip[INET6_ADDRSTRLEN];
    int csock;

    if ((csock = z_socket_tcp_accept(Z_IOPOLL_ENTITY_FD(server), NULL)) < 0)
        return(-1);

    z_socket_address(csock, &address);
    z_socket_str_address(ip, INET6_ADDRSTRLEN, (struct sockaddr *)&address);
    fprintf(stderr, " - WebSock Accept %d %s.\n", csock, ip);

    return(csock);
}

typedef enum handshake_headers {
    HANDSHAKE_HEADER_PATH       = 0,
    HANDSHAKE_HEADER_LOCATION   = 1,
    HANDSHAKE_HEADER_HOST       = 2,
    HANDSHAKE_HEADER_KEY        = 3,
    HANDSHAKE_HEADER_VERSION    = 4,
    HANDSHAKE_HEADER_EXTENSIONS = 5,
    HANDSHAKE_HEADER_COUNT      = 6,
} handshake_headers_t;

static unsigned int __websocket_key (char *buffer, const z_chunkq_extent_t *key)
{
    unsigned int koffset;
    unsigned int klength;
    char sha1key[512];
    uint8_t sha1[20];

    koffset = key->offset + 19;
    klength = key->length - 19;

    /* Sec-WebSocket-Key + 258EAFA5-E914-47DA-95CA-C5AB0DC85B11 */
    z_chunkq_read(key->chunkq, koffset, sha1key, klength);
    z_memcpy(sha1key + klength, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 36);
    sha1key[klength + 36] = '\0';
    printf("SHA1 KEY %s\n", sha1key);

    z_hash160_sha1(sha1, sha1key, klength + 36);

    return(z_base64_encode(buffer, sha1, 20));
}
#include <stdio.h>
static int __websock_handshake (z_rpc_client_t *client, long end) {
    z_chunkq_extent_t headers[HANDSHAKE_HEADER_COUNT];
    z_chunkq_extent_t extent;
    unsigned int offset;
    struct iovec iov[6];
    char buffer[32];
    unsigned int n;

    /* Parse request */
    offset = 0;
    while (z_rpc_tokenize_line(client, &extent, offset)) {
        offset = extent.offset + extent.length + 1;

        if (!__key_cmp(client, &extent, "host", 4))
            z_chunkq_extent_copy(&(headers[HANDSHAKE_HEADER_HOST]), &extent);
        if (!__key_cmp(client, &extent, "location", 8))
            z_chunkq_extent_copy(&(headers[HANDSHAKE_HEADER_LOCATION]), &extent);
        else if (!__key_cmp(client, &extent, "sec-websocket-key", 17))
            z_chunkq_extent_copy(&(headers[HANDSHAKE_HEADER_KEY]), &extent);
        else if (!__key_cmp(client, &extent, "sec-websocket-extensions", 24))
            z_chunkq_extent_copy(&(headers[HANDSHAKE_HEADER_EXTENSIONS]), &extent);
        else if (!__key_cmp(client, &extent, "sec-websocket-version", 21))
            z_chunkq_extent_copy(&(headers[HANDSHAKE_HEADER_VERSION]), &extent);
        else if (!__key_cmp(client, &extent, "GET /", 5))
            z_chunkq_extent_copy(&(headers[HANDSHAKE_HEADER_PATH]), &extent); /* TODO */

/* 
        z_chunkq_read(&(client->rdbuffer), extent.offset, buffer, extent.length);
        buffer[extent.length] = 0;
        printf("TOKENIZE %u:%u '%s'...\n", extent.offset, extent.length, buffer);
*/
    }

    /* Send response */
    iov[0].iov_base = "HTTP/1.1 101 Switching Protocols\r\n";
    iov[0].iov_len = 34;

    iov[1].iov_base = "Upgrade: websocket\r\n";
    iov[1].iov_len  = 20;

    iov[2].iov_base = "Connection: Upgrade\r\n";
    iov[2].iov_len  = 21;

    iov[3].iov_base = "Sec-WebSocket-Accept: ";
    iov[3].iov_len = 22;

    n = __websocket_key(buffer, &(headers[HANDSHAKE_HEADER_KEY]));
    buffer[n] = '\0';
    printf("key: %u %s\n", n, buffer);
    iov[4].iov_base = buffer;
    iov[4].iov_len = n;

    iov[5].iov_base = "\r\n\r\n";
    iov[5].iov_len  = 4;

    z_rpc_writev(client, iov, 6);

    return(0);
}

static int __websock_process (z_rpc_client_t *client) {
    char buffer[512];    
    long n;

    n = z_chunkq_read(&(client->rdbuffer), 0, buffer, sizeof(buffer));
    buffer[n] = 0;
    printf("BUFFER %ld '%s'...\n", n, buffer);

    if ((n = z_chunkq_indexof(&(client->rdbuffer), 0, "\r\n\r\n", 4)) > 0) {
        printf("DO HAND SHAKE\n");
        __websock_handshake(client, n);
        z_chunkq_remove(&(client->rdbuffer), n);
        return(0);
    }

    //z_chunkq_append(&(client->wrbuffer), "OK\r\n", 2);
    return(0);
}

z_rpc_protocol_t websock_protocol = {
    .bind           = __websock_bind,
    .accept         = __websock_accept,
    .connected      = NULL,
    .disconnected   = NULL,
    .process        = __websock_process,
};

