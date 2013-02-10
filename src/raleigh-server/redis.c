#include <zcl/humans.h>
#include <zcl/slice.h>
#include <zcl/iobuf.h>
#include <zcl/ipc.h>

#include <unistd.h>
#include <stdio.h>

#include "server.h"

static int __iobuf_write_buf (struct redis_client *client,
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

static int __process_ping (struct redis_client *client,
                           z_iobuf_reader_t *reader,
                           z_slice_t *slice)
{
    return(__iobuf_write_buf(client, "+PONG\r\n", 7));
}

static int __process_stat (struct redis_client *client,
                           z_iobuf_reader_t *reader,
                           z_slice_t *slice)
{
    z_iopoll_stats_t *stats = &(z_ipc_client_iopoll(client)->stats);
    char buf0[16], buf1[16], buf2[16];
    char buf3[16], buf4[16], buf5[16];
    char buf6[16], buf7[16], buf8[16];
    char sbuf[512];
    unsigned int n;

    z_iopoll_stats_dump(z_ipc_client_iopoll(client));
    n = snprintf(sbuf, sizeof(sbuf),
        "max events:     %u\n"
        "poll swtich:    %lu\n"
        "read events:    %lu\n"
        "write events:   %lu\n"
        "avg IO wait:    %s (%s-%s MIN %lu MAX %lu)\n"
        "avg IO read:    %s (%s-%s MIN %lu MAX %lu)\n"
        "avg IO write:   %s (%s-%s MIN %lu MAX %lu)\n",
        stats->max_events,
        stats->iowait.nevents,
        stats->ioread.nevents,
        stats->iowrite.nevents,
        z_human_time(buf0, sizeof(buf0), z_histogram_average(&(stats->iowait))),
        z_human_time(buf1, sizeof(buf1), z_histogram_percentile(&(stats->iowait), 0)),
        z_human_time(buf2, sizeof(buf2), z_histogram_percentile(&(stats->iowait), 99.9999)),
        stats->iowait.min, stats->iowait.max,
        z_human_time(buf3, sizeof(buf3), z_histogram_average(&(stats->ioread))),
        z_human_time(buf4, sizeof(buf4), z_histogram_percentile(&(stats->ioread), 0)),
        z_human_time(buf5, sizeof(buf5), z_histogram_percentile(&(stats->ioread), 99.9999)),
        stats->ioread.min, stats->ioread.max,
        z_human_time(buf6, sizeof(buf6), z_histogram_average(&(stats->iowrite))),
        z_human_time(buf7, sizeof(buf7), z_histogram_percentile(&(stats->iowrite), 0)),
        z_human_time(buf8, sizeof(buf8), z_histogram_percentile(&(stats->iowrite), 99.9999)),
        stats->iowrite.min, stats->iowrite.max);

    return(__iobuf_write_buf(client, sbuf, n));
}

static int __process_quit (struct redis_client *client,
                           z_iobuf_reader_t *reader,
                           z_slice_t *slice)
{
    __iobuf_write_buf(client, "+OK\r\n", 5);
    return(-1);
}

/* ===========================================================================
 *  Redis protocol
 */
typedef int (*redis_func_t) (struct redis_client *client,
                             z_iobuf_reader_t *reader,
                             z_slice_t *slice);

struct redis_command {
    const char * name;
    unsigned int length;
    redis_func_t func;
};

static const struct redis_command __redis_commands[] = {
    { "ping", 4, __process_ping },
    { "quit", 4, __process_quit },
    { "stat", 4, __process_stat },

    { "PING", 4, __process_ping },
    { "QUIT", 4, __process_quit },
    { "STAT", 4, __process_stat },

    { NULL, 0, NULL },
};

static int __redis_process (struct redis_client *client) {
    const struct redis_command *p;
    z_iobuf_reader_t reader;
    size_t n, ndrain = 0;
    z_slice_t slice;

    z_slice_alloc(&slice, z_ipc_client_memory(client));
    z_reader_open(&reader, &(client->ibuffer));
    while ((n = z_reader_tokenize(&reader, " \r\n", 3, &slice)) > 0) {
        int result = -1;

        ndrain += n;
        if (z_slice_is_empty(&slice)) continue;

        for (p = __redis_commands; p->name != NULL; ++p) {
            if (n < p->length) continue;
            if (z_slice_equals(&slice, p->name, p->length)) {
                result = p->func(client, &reader, &slice);
                break;
            }
        }

        if (result < 0) {
            printf("NOT FOUND %lu:", z_slice_length(&slice));
            z_slice_dump(&slice);
            return(result);
        }
    }
    z_iobuf_drain(&(client->ibuffer), ndrain);
    z_reader_close(&reader);
    z_ipc_client_set_writable(client, 1);
    return(0);
}

/* ===========================================================================
 *  IPC protocol handlers
 */
static int __client_connected (z_ipc_client_t *client) {
    struct redis_client *redis = (struct redis_client *)client;

    printf("redis client connected\n");
    z_iobuf_alloc(&(redis->ibuffer), client->server->memory, 128);
    z_iobuf_alloc(&(redis->obuffer), client->server->memory, 128);
    return(0);
}

static void __client_disconnected (z_ipc_client_t *client) {
    struct redis_client *redis = (struct redis_client *)client;

    printf("redis client disconnected\n");
    z_iobuf_free(&(redis->ibuffer));
    z_iobuf_free(&(redis->obuffer));
}

static int __client_read (z_ipc_client_t *client) {
    struct redis_client *redis = (struct redis_client *)client;
    unsigned char *buffer;
    ssize_t n;

    while ((n = z_iobuf_write(&(redis->ibuffer), &buffer)) > 0) {
        ssize_t rd;
        if ((rd = read(Z_IOPOLL_ENTITY_FD(client), buffer, n)) < n) {
            z_iobuf_write_backup(&(redis->ibuffer), n - rd);
            break;
        }
    }

    return(__redis_process(redis));
}

static int __client_write (z_ipc_client_t *client) {
    struct redis_client *redis = (struct redis_client *)client;
    unsigned char *buffer;
    ssize_t n;

    while ((n = z_iobuf_read(&(redis->obuffer), &buffer)) > 0) {
        ssize_t wr;
        if ((wr = write(Z_IOPOLL_ENTITY_FD(client), buffer, n)) < n) {
            z_iobuf_read_backup(&(redis->obuffer), n - wr);
            break;
        }
    }

    z_iopoll_set_writable(client->server->iopoll, Z_IOPOLL_ENTITY(client), 0);
    return(0);
}

const struct z_ipc_protocol redis_protocol = {
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