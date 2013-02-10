#ifndef _RALEIGH_SERVER_H_
#define _RALEIGH_SERVER_H_

#include <zcl/iobuf.h>
#include <zcl/ipc.h>

struct echo_client {
    __Z_IPC_CLIENT__
    z_iobuf_t buffer;
};

struct redis_client {
    __Z_IPC_CLIENT__
    z_iobuf_t ibuffer;
    z_iobuf_t obuffer;
};

struct msgbuf_node {
    volatile unsigned int refs;
    struct msgbuf_node *next;
    unsigned int available;
    uint8_t data[0];
};

struct msgbuf {
    z_memory_t *memory;
    struct msgbuf_node *head;
    struct msgbuf_node *tail;
    uint64_t length;
    uint32_t offset;
    int refs;
};

struct raleighfs_client {
    __Z_IPC_CLIENT__
    z_ipc_msgbuf_t msgbuf;
    z_iobuf_t obuffer;
    uint16_t field_id;
};

struct stats_client {
    __Z_IPC_CLIENT__
    z_ipc_msgbuf_t imsgbuf;
    z_iobuf_t obuffer;
};


extern const z_ipc_protocol_t echo_protocol;
extern const z_ipc_protocol_t redis_protocol;
extern const z_ipc_protocol_t stats_protocol;
extern const z_ipc_protocol_t raleighfs_protocol;

#define z_ipc_echo_plug(memory, iopoll, address, service, udata)              \
    z_ipc_plug(memory, iopoll, &echo_protocol, struct echo_client,            \
        address, service, udata)

#define z_ipc_redis_plug(memory, iopoll, address, service, udata)             \
    z_ipc_plug(memory, iopoll, &redis_protocol, struct redis_client,          \
        address, service, udata)

#define z_ipc_raleighfs_plug(memory, iopoll, address, service, udata)         \
    z_ipc_plug(memory, iopoll, &raleighfs_protocol, struct raleighfs_client,  \
        address, service, udata)

#define z_ipc_stats_plug(memory, iopoll, address, service, udata)             \
    z_ipc_plug(memory, iopoll, &stats_protocol, struct stats_client,          \
        address, service, udata)

#endif /* !_RALEIGH_SERVER_H_ */
