#include <zcl/coding.h>
#include <zcl/string.h>
#include <zcl/debug.h>
#include <zcl/iobuf.h>
#include <zcl/ipc.h>

#include <unistd.h>
#include <stdio.h>

#include "server.h"

static int __iobuf_write_buf (struct stats_client *client,
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

static int __client_connected (z_ipc_client_t *ipc_client) {
    struct stats_client *client = (struct stats_client *)ipc_client;
    z_ipc_msgbuf_open(&(client->imsgbuf), z_ipc_client_memory(client));
    z_iobuf_alloc(&(client->obuffer), z_ipc_client_memory(client), 128);
    Z_LOG_DEBUG("Stats client connected\n");
    return(0);
}

static void __client_disconnected (z_ipc_client_t *ipc_client) {
    struct stats_client *client = (struct stats_client *)ipc_client;
    Z_LOG_DEBUG("Stats client disconnected\n");
    z_ipc_msgbuf_close(&(client->imsgbuf));
    z_iobuf_free(&(client->obuffer));
}

static int __client_read (z_ipc_client_t *ipc_client) {
    struct stats_client *client = (struct stats_client *)ipc_client;
    z_ipc_msg_t msg;
    while (z_ipc_msgbuf_add(&(client->imsgbuf), Z_IOPOLL_ENTITY_FD(client)) > 0) {
        while (z_ipc_msgbuf_get(&(client->imsgbuf), &msg) != NULL) {
            // IOPOLL
            // SYSTEM

            // Stack CPU: ru_utime, ru_stime
            // Stack I/O: ru_inblock, ru_oublock
            // Stack IPC: ru_msgsnd, ru_msgrcv
            // Stack Switch: ru_nvcsw, ru_nivcsw
#if 0
            getrusage(RUSAGE_SELF, &usage);
           struct rusage {
               struct timeval ru_utime; /* user CPU time used */
               struct timeval ru_stime; /* system CPU time used */
               long   ru_maxrss;        /* maximum resident set size */
               long   ru_ixrss;         /* integral shared memory size */
               long   ru_idrss;         /* integral unshared data size */
               long   ru_isrss;         /* integral unshared stack size */
               long   ru_minflt;        /* page reclaims (soft page faults) */
               long   ru_majflt;        /* page faults (hard page faults) */
               long   ru_nswap;         /* swaps */
               long   ru_inblock;       /* block input operations */
               long   ru_oublock;       /* block output operations */
               long   ru_msgsnd;        /* IPC messages sent */
               long   ru_msgrcv;        /* IPC messages received */
               long   ru_nsignals;      /* signals received */
               long   ru_nvcsw;         /* voluntary context switches */
               long   ru_nivcsw;        /* involuntary context switches */
           };
#endif

            /* send response */
            unsigned char buf[16];
            int elen = z_encode_vint(buf, 10);
            z_memcpy(buf + elen, "+ok012345!", 10);
            __iobuf_write_buf(client, buf, elen + 10);
            z_ipc_client_set_writable(client, 1);

            z_ipc_msg_free(&msg);
        }
    }
    return(0);
}

static int __client_write (z_ipc_client_t *ipc_client) {
    struct stats_client *client = (struct stats_client *)ipc_client;
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

const struct z_ipc_protocol stats_protocol = {
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
