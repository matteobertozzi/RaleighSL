#include <zcl/iobuf.h>
#include <zcl/ipc.h>

#include <unistd.h>
#include <stdio.h>

#include "server.h"

static int __client_connected (z_ipc_client_t *client) {
    struct echo_client *echo = (struct echo_client *)client;

    printf("client connected\n");
    z_iobuf_alloc(&(echo->buffer), client->server->memory, 2048);
    return(0);
}

static void __client_disconnected (z_ipc_client_t *client) {
    struct echo_client *echo = (struct echo_client *)client;

    printf("client disconnected\n");
    z_iobuf_free(&(echo->buffer));
}

static int __client_read (z_ipc_client_t *client) {
    struct echo_client *echo = (struct echo_client *)client;
    unsigned char *buffer;
    ssize_t n;

    while ((n = z_iobuf_write(&(echo->buffer), &buffer)) > 0) {
        ssize_t rd;
        if ((rd = read(Z_IOPOLL_ENTITY_FD(client), buffer, n)) < n) {
            z_iobuf_write_backup(&(echo->buffer), n - rd);
            break;
        }
    }

    z_ipc_client_set_writable(client, 1);
    return(0);
}

static int __client_write (z_ipc_client_t *client) {
    struct echo_client *echo = (struct echo_client *)client;
    unsigned char *buffer;
    ssize_t n;

    while ((n = z_iobuf_read(&(echo->buffer), &buffer)) > 0) {
        ssize_t wr;
        if ((wr = write(Z_IOPOLL_ENTITY_FD(client), buffer, n)) < n) {
            z_iobuf_read_backup(&(echo->buffer), n - wr);
            break;
        }
    }

    z_ipc_client_set_writable(client, 0);
    return(0);
}

const struct z_ipc_protocol echo_protocol = {
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