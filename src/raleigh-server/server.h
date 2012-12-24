#ifndef _RALEIGH_SERVER_H_
#define _RALEIGH_SERVER_H_

#include <zcl/iobuf.h>
#include <zcl/ipc.h>

struct echo_client {
    __Z_IPC_CLIENT__
    z_iobuf_t buffer;
};

extern const z_ipc_protocol_t echo_protocol;

#define z_ipc_echo_plug(memory, iopoll, address, service, udata)             \
    z_ipc_plug(memory, iopoll, &echo_protocol, struct echo_client,           \
        address, service, udata)

#endif /* !_RALEIGH_SERVER_H_ */
