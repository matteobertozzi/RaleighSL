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

#include <zcl/eloop.h>
#include <zcl/ipc.h>

typedef struct r5l_server r5l_server_t;

struct r5l_server {
  z_event_loop_t eloop[1];
  int term_signum;
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

extern const z_msg_protocol_t log_msg_protocol;

extern const z_ipc_protocol_t log_tcp_protocol;
extern const z_ipc_protocol_t echo_tcp_protocol;
extern const z_ipc_protocol_t udo_udp_protocol;

#define __ipc_proto_plug(eloop, srv_type, proto_type, address, service, udata) \
  z_ipc_plug(&((eloop)->iopoll), &((eloop)->memory),                           \
             & srv_type ## _ ## proto_type ## _protocol, NULL,                 \
             # srv_type, struct srv_type ## _client, address, service, udata)

#define __ipc_msg_proto_plug(eloop, srv_type, proto_type,                      \
                             address, service, udata)                          \
  z_ipc_plug(&((eloop)->iopoll), &((eloop)->memory),                           \
             & srv_type ## _ ## proto_type ## _protocol,                       \
             & srv_type ## _msg_protocol,                                      \
             # srv_type, struct srv_type ## _client, address, service, udata)

#define echo_tcp_plug(eloop, addr, port, udata)                                \
  __ipc_proto_plug(eloop, echo, tcp, addr, port, udata)

#define udo_udp_plug(eloop, addr, port, udata)                                 \
  __ipc_proto_plug(eloop, udo, udp, addr, port, udata)

#define log_tcp_plug(eloop, addr, port, udata)                                 \
  __ipc_msg_proto_plug(eloop, log, tcp, addr, port, udata)

r5l_server_t *r5l_server_open         (void);
void          r5l_server_close        (r5l_server_t *server);
void          r5l_server_stop_signal  (r5l_server_t *server, int signum);

__Z_END_DECLS__

#endif /* !_R5L_MAIN_SERVER_H_ */
