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

#ifndef _RALEIGH_CLIENT_PRIVATE_H_
#define _RALEIGH_CLIENT_PRIVATE_H_

#include <zcl/global.h>
#include <zcl/socket.h>
#include <zcl/iopoll.h>
#include <zcl/rpc.h>

struct raleigh_client {
  __Z_IPC_CLIENT__
  z_ipc_msgbuf_t msgbuf;
  z_rpc_map_t    rpc_map;
  uint64_t       req_id;
};

typedef struct raleigh_global {
  int is_running;
  z_allocator_t allocator;
  z_iopoll_t iopoll;
} raleigh_global_t;

extern raleigh_global_t __global_ctx;

#define __RALEIGH_USE_ASYNC_AND_LATCH       1

#define __raleigh_call(type, callback, udata, ...)						                       \
	((type)callback)(udata, ##__VA_ARGS__)

#define __raleigh_client_push_request(client, ctx, sys_callback, ucallback, udata)	 \
	raleighsl_rpc_client_push_request(&(__global_ctx.iopoll), ctx, 							       \
																	  &((client)->msgbuf), &((client)->rpc_map),       \
                                    sys_callback, ucallback, udata)

#endif /* !_RALEIGH_CLIENT_PRIVATE_H_ */
