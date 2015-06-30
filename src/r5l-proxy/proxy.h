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

#ifndef _R5L_PROXY_H_
#define _R5L_PROXY_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/eloop.h>
#include <zcl/ipc.h>
#include <zcl/tls.h>

typedef struct r5l_proxy r5l_proxy_t;
typedef struct r5l_proxy_client r5l_proxy_client_t;

#define R5L_PROXY_SERVER(x)         Z_CAST(r5l_proxy_t, x)
#define R5L_PROXY_CLIENT(x)         Z_CAST(r5l_proxy_client_t, x)

struct r5l_proxy {
  z_event_loop_t eloop[1];
  int term_signum;

  uint64_t next_cid;
  // client-map cid:ptr

  uint32_t ringmask;
  uint32_t __pad;

  uint8_t *ringbuf;

  z_tls_ctx_t *tls;
};

struct r5l_proxy_client {
  __Z_IPC_MSG_CLIENT__
  uint64_t client_id;
  uint8_t buffer[128];
  unsigned int size;
  z_tls_t *tls;
};

extern const z_msg_protocol_t r5l_proxy_msg_protocol;

extern const z_ipc_protocol_t r5l_proxy_unix_protocol;
extern const z_ipc_protocol_t r5l_proxy_tls_protocol;
extern const z_ipc_protocol_t r5l_proxy_tcp_protocol;
extern const z_ipc_protocol_t r5l_proxy_udp_protocol;

#define __ipc_proto_plug(eloop, srv_type, proto_type, address, service, udata) \
  z_ipc_plug(&((eloop)->iopoll), &((eloop)->memory),                           \
             & srv_type ## _ ## proto_type ## _protocol, NULL,                 \
             # srv_type, struct srv_type ## _client, address, service, udata)

#define __ipc_msg_proto_plug(eloop, srv_type, proto_type,                     \
                             address, service, udata)                         \
  z_ipc_plug(&((eloop)->iopoll), &((eloop)->memory),                          \
             & srv_type ## _ ## proto_type ## _protocol,                      \
             & srv_type ## _msg_protocol,                                     \
             # srv_type, struct srv_type ## _client, address, service, udata)

#define r5l_proxy_udp_plug(eloop, addr, port, udata)                          \
  __ipc_proto_plug(eloop, r5l_proxy, udp, addr, port, udata)

#define r5l_proxy_tls_plug(eloop, addr, port, udata)                          \
  __ipc_proto_plug(eloop, r5l_proxy, tls, addr, port, udata)

#define r5l_proxy_tcp_plug(eloop, addr, port, udata)                          \
  __ipc_msg_proto_plug(eloop, r5l_proxy, tcp, addr, port, udata)

#define r5l_proxy_unix_plug(eloop, path, udata)                               \
  __ipc_msg_proto_plug(eloop, r5l_proxy, unix, path, NULL, udata)

r5l_proxy_t *r5l_proxy_open         (void);
void         r5l_proxy_close        (r5l_proxy_t *proxy);
void         r5l_proxy_stop_signal  (r5l_proxy_t *proxy, int signum);

__Z_END_DECLS__

#endif /* !_R5L_PROXY_H_ */
