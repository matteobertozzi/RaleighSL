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

#ifndef _RALEIGH_SERVER_H_
#define _RALEIGH_SERVER_H_

#include <raleighfs/raleighfs.h>

#include <zcl/ringbuf.h>
#include <zcl/ipc.h>

#define SERVER_CONTEXT(x)           Z_CAST(struct server_context, x)
#define ECHO_CLIENT(x)              Z_CAST(struct echo_client, x)
#define REDIS_CLIENT(x)             Z_CAST(struct redis_client, x)
#define RALEIGHFS_CLIENT(x)         Z_CAST(struct raleighfs_client, x)
#define STATS_CLIENT(x)             Z_CAST(struct stats_client, x)

struct server_context {
  int is_running;
  raleighfs_t fs;
  z_allocator_t allocator;
  z_iopoll_t iopoll;
};

struct echo_client {
  __Z_IPC_CLIENT__
  z_ringbuf_t buffer;
};

struct redis_client {
  __Z_IPC_CLIENT__
  z_ringbuf_t ibuffer;
  z_ringbuf_t obuffer;
};

struct raleighfs_client {
  __Z_IPC_CLIENT__
  z_ipc_msgbuf_t msgbuf;
};

struct stats_client {
  __Z_IPC_CLIENT__
  z_ipc_msgbuf_t msgbuf;
  uint64_t id;
};

extern const z_ipc_protocol_t echo_protocol;
extern const z_ipc_protocol_t redis_protocol;
extern const z_ipc_protocol_t stats_protocol;
extern const z_ipc_protocol_t raleighfs_protocol;

#define z_ipc_echo_plug(iopoll, address, service, udata)              \
  z_ipc_plug(iopoll, &echo_protocol, struct echo_client,              \
             address, service, udata)

#define z_ipc_redis_plug(iopoll, address, service, udata)             \
  z_ipc_plug(iopoll, &redis_protocol, struct redis_client,            \
             address, service, udata)

#define z_ipc_stats_plug(iopoll, address, service, udata)             \
  z_ipc_plug(iopoll, &stats_protocol, struct stats_client,            \
             address, service, udata)

#define z_ipc_raleighfs_plug(iopoll, address, service, udata)         \
  z_ipc_plug(iopoll, &raleighfs_protocol, struct raleighfs_client,    \
             address, service, udata)


#endif /* !_RALEIGH_SERVER_H_ */
