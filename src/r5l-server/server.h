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

#include <zcl/ipc.h>

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

extern const z_ipc_protocol_t echo_tcp_protocol;
extern const z_ipc_protocol_t udo_udp_protocol;

__Z_END_DECLS__

#endif /* !_R5L_MAIN_SERVER_H_ */
