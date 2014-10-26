/*
 *   Copyright 2007-2013 Matteo Bertozzi
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#ifndef _R5L_MAIN_SERVER_H_
#define _R5L_MAIN_SERVER_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/allocator.h>
#include <zcl/global.h>
#include <zcl/ipc.h>

Z_TYPEDEF_STRUCT(metrics_rpc_task)

#define SERVER_CONTEXT(x)           Z_CAST(struct server_context, x)
#define ECHO_CLIENT(x)              Z_CAST(struct echo_client, x)

#define __global_server_context()           \
  Z_CAST(struct server_context, z_global_udata())

enum server_type {
  ECHO_SERVER     = 0,
  METRICS_SERVER  = 1,
  UDO_SERVER      = 2,
  R5L_SERVER      = 3,
};

struct server_context {
  z_ipc_server_t *icp_server[4];
};

struct echo_client {
  __Z_IPC_CLIENT__
  uint8_t buffer[128];
  unsigned int size;
  uint64_t reads;
  uint64_t writes;
  uint64_t st;
};

struct udo_client {
};

struct metrics_client {
  __Z_IPC_MSG_CLIENT__
  metrics_rpc_task_t *task;
};

struct r5l_client {
  __Z_IPC_MSG_CLIENT__
};

extern const z_ipc_msg_protocol_t metrics_msg_protocol;
extern const z_ipc_msg_protocol_t r5l_msg_protocol;

extern const z_ipc_protocol_t metrics_tcp_protocol;
extern const z_ipc_protocol_t echo_tcp_protocol;
extern const z_ipc_protocol_t udo_udp_protocol;
extern const z_ipc_protocol_t r5l_tcp_protocol;

#define __ipc_proto_plug(srv_type, proto_type, address, service)              \
  z_ipc_plug(& srv_type ## _ ## proto_type ## _protocol, NULL,                \
             # srv_type, struct srv_type ## _client, address, service)

#define __ipc_msg_proto_plug(srv_type, proto_type, address, service)          \
  z_ipc_plug(& srv_type ## _ ## proto_type ## _protocol,                      \
             & srv_type ## _msg_protocol,                                     \
             # srv_type, struct srv_type ## _client, address, service)

#define metrics_tcp_plug(addr, port)  __ipc_msg_proto_plug(metrics, tcp, addr, port)
#define echo_tcp_plug(addr, port)     __ipc_proto_plug(echo, tcp, addr, port)
#define udo_udp_plug(addr, port)      __ipc_proto_plug(udo, udp, addr, port)
#define r5l_tcp_plug(addr, port)      __ipc_msg_proto_plug(r5l, tcp, addr, port)

__Z_END_DECLS__

#endif /* !_R5L_MAIN_SERVER_H_ */
