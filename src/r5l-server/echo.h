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

#ifndef _R5L_SERVER_ECHO_H_
#define _R5L_SERVER_ECHO_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/ipc.h>

typedef struct echo_server {
  z_ipc_server_t server;
} echo_server_t;

struct echo_client {
  z_ipc_client_t ipc_client;
  uint8_t buffer[128];
  unsigned int size;
  uint64_t reads;
  uint64_t writes;
  uint64_t st;
};

extern const z_ipc_protocol_t echo_tcp_protocol;

__Z_END_DECLS__

#endif /* !_R5L_SERVER_ECHO_H_ */
