/*
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

#ifndef _R5L_SERVER_H_
#define _R5L_SERVER_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/memory.h>
#include <zcl/iopoll.h>
#include <zcl/ipc.h>

#include "metrics.h"
#include "echo.h"

typedef struct msgtest_server {
  z_ipc_server_t server;
} msgtest_server_t;

typedef struct msgtest_client {
  z_ipc_msg_client_t ipc_client;
  uint8_t buffer[128];
} msgtest_client_t;

extern const z_ipc_msg_protocol_t msgtest_msg_protocol;
extern const z_ipc_protocol_t msgtest_tcp_protocol;

typedef struct r5l_server {
  z_iopoll_engine_t iopoll;
  z_memory_t memory;
  z_allocator_t allocator;

  z_ipc_msg_stats_t ipc_msg_req_stats;
  z_ipc_msg_stats_t ipc_msg_resp_stats;

  metrics_server_t metrics;
  echo_server_t echo;
  msgtest_server_t msgtest;
} r5l_server_t;

#define r5l_ipc_proto_plug(self, srv_type, proto_type, addr, service)          \
  z_ipc_plug(&((self)->srv_type.server), &((self)->iopoll), &((self)->memory), \
             & srv_type ## _ ## proto_type ## _protocol, NULL,                 \
             # srv_type, struct srv_type ## _client, addr, service, self)

#define r5l_ipc_msg_proto_plug(self, srv_type, proto_type, addr, service)      \
  z_ipc_plug(&((self)->srv_type.server), &((self)->iopoll), &((self)->memory), \
              & srv_type ## _ ## proto_type ## _protocol,                      \
              & srv_type ## _msg_protocol,                                     \
              # srv_type, struct srv_type ## _client, addr, service, self)

#define r5l_ipc_proto_unplug(self, srv_type)                                   \
  z_ipc_unplug(&((self)->srv_type.server))

#define echo_tcp_plug(server, addr, port)                               \
  r5l_ipc_proto_plug(server, echo, tcp, addr, port)

#define metrics_tcp_plug(server, addr, port)                            \
  r5l_ipc_msg_proto_plug(server, metrics, tcp, addr, port)

#define msgtest_tcp_plug(server, addr, port)                            \
  r5l_ipc_msg_proto_plug(server, msgtest, tcp, addr, port)

#define echo_tcp_unplug(server)        r5l_ipc_proto_unplug(server, echo)
#define metrics_tcp_unplug(server)     r5l_ipc_proto_unplug(server, metrics)
#define msgtest_tcp_unplug(server)     r5l_ipc_proto_unplug(server, msgtest)

__Z_END_DECLS__

#endif /* !_R5L_SERVER_H_ */
