#include <zcl/coding.h>
#include <zcl/string.h>
#include <zcl/debug.h>
#include <zcl/iobuf.h>
#include <zcl/time.h>
#include <zcl/ipc.h>

#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>

#include "rpc/generated/stats.h"
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

static int __write_framed_buffer (struct stats_client *client,
                                  const unsigned char *buffer,
                                  unsigned int length)
{
    unsigned char fbuf[16];
    int elen;

    elen = z_encode_vint(fbuf, length);
    __iobuf_write_buf(client, fbuf, elen);
    __iobuf_write_buf(client, buffer, length);

    return(0);
}

static int __client_read (z_ipc_client_t *ipc_client) {
    struct stats_client *client = (struct stats_client *)ipc_client;
    z_ipc_msg_t msg;

    while (z_ipc_msgbuf_add(&(client->imsgbuf), Z_IOPOLL_ENTITY_FD(client)) > 0) {
        while (z_ipc_msgbuf_get(&(client->imsgbuf), &msg) != NULL) {
            unsigned char ebuffer[512];
            unsigned char *buf = ebuffer;
            struct rusage usage;

            z_ipc_msg_free(&msg);

            // SYSTEM
            getrusage(RUSAGE_SELF, &usage);

            // Stack CPU: ru_utime, ru_stime
            buf = rpc_rusage_write_utime(buf, z_timeval_to_micros(&(usage.ru_utime)));
            buf = rpc_rusage_write_stime(buf, z_timeval_to_micros(&(usage.ru_stime)));

            // Memory
            buf = rpc_rusage_write_maxrss(buf, usage.ru_maxrss << 10);

            // Stack I/O: ru_inblock, ru_oublock
            buf = rpc_rusage_write_minflt(buf, usage.ru_minflt);
            buf = rpc_rusage_write_majflt(buf, usage.ru_majflt);
            buf = rpc_rusage_write_inblock(buf, usage.ru_inblock);
            buf = rpc_rusage_write_oublock(buf, usage.ru_oublock);

            // Stack Switch: ru_nvcsw, ru_nivcsw
            buf = rpc_rusage_write_nvcsw(buf, usage.ru_nvcsw);
            buf = rpc_rusage_write_nivcsw(buf, usage.ru_nivcsw);


            // IOPOLL
            z_iopoll_stats_t *stats = &(z_ipc_client_iopoll(client)->engines[0].stats);
            buf = rpc_rusage_write_iowait(buf, stats->iowait.sum);
            buf = rpc_rusage_write_ioread(buf, stats->ioread.sum);
            buf = rpc_rusage_write_iowrite(buf, stats->iowrite.sum);

            /* send response */
            __write_framed_buffer(client, ebuffer, buf - ebuffer);
            z_ipc_client_set_writable(client, 1);
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
