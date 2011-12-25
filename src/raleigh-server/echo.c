#include <zcl/iopoll.h>
#include <zcl/chunkq.h>
#include <zcl/socket.h>
#include <zcl/rpc.h>

#include <unistd.h>
#include <ctype.h>
#include <stdio.h>

/* ============================================================================
 *  Echo Protocol Methods
 */
static int __echo_bind (const void *hostname, const void *service) {
    int socket;

    if ((socket = z_socket_tcp_bind(hostname, service, NULL)) < 0)
        return(-1);

    fprintf(stderr, " - Echo Interface up and running on %s:%s...\n",
            (const char *)hostname, (const char *)service);

    return(socket);
}

static int __echo_accept (z_rpc_server_t *server) {
    struct sockaddr_storage address;
    char ip[INET6_ADDRSTRLEN];
    int csock;

    if ((csock = z_socket_tcp_accept(Z_IOPOLL_ENTITY_FD(server), NULL)) < 0)
        return(-1);

    z_socket_address(csock, &address);
    z_socket_str_address(ip, INET6_ADDRSTRLEN, (struct sockaddr *)&address);
    fprintf(stderr, " - Echo Accept %d %s.\n", csock, ip);

    return(csock);
}

static int __echo_process (z_rpc_client_t *client) {
    z_chunkq_append(&(client->wrbuffer), "OK", 2);
    return(1);
}

z_rpc_protocol_t echo_protocol = {
    .bind           = __echo_bind,
    .accept         = __echo_accept,
    .connected      = NULL,
    .disconnected   = NULL,
    .process        = __echo_process,
};

