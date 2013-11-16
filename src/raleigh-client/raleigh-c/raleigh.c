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

#include <zcl/global.h>
#include <zcl/socket.h>
#include <zcl/iopoll.h>
#include <zcl/rpc.h>

#include "generated/rpc_client.h"
#include "raleigh.h"
#include "private.h"

/* ============================================================================
 *  PRIVATE Global Context data
 */
raleigh_global_t __global_ctx;

/* ============================================================================
 *  PRIVATE Client vtable
 */
#if 0
struct __ = {
  .error_handling  = NULL,
  .server_request  = NULL,
  .unknown_message = NULL,
};
#endif

static int __client_msg_parse (z_iopoll_entity_t *ipc_client, const struct iovec iov[2]) {
  raleigh_client_t *client = RALEIGH_CLIENT(ipc_client);
  /* TODO: client messages, server messages, unknown messages */
  return(raleighsl_rpc_client_parse(ipc_client, &(client->rpc_map), iov));
}

static void __ipc_client_close (z_iopoll_entity_t *ipc_client) {
  raleigh_client_t *client = RALEIGH_CLIENT(ipc_client);

  Z_LOG_DEBUG("Raleigh client disconnected");
  z_ipc_msgbuf_close(&(client->msgbuf));

  /* Close client description */
  z_iopoll_entity_close(ipc_client);
}

static int __ipc_client_read (z_iopoll_entity_t *ipc_client) {
  raleigh_client_t *client = RALEIGH_CLIENT(ipc_client);
  return(z_ipc_msgbuf_fetch(&(client->msgbuf), ipc_client, __client_msg_parse));
}

static int __ipc_client_write (z_iopoll_entity_t *ipc_client) {
  raleigh_client_t *client = RALEIGH_CLIENT(ipc_client);
  Z_LOG_DEBUG("client write");
  return(z_ipc_msgbuf_flush(&(client->msgbuf), &(__global_ctx.iopoll), ipc_client));
}

static const z_vtable_iopoll_entity_t __ipc_client_vtable = {
  .write = __ipc_client_write,
  .read  = __ipc_client_read,
  .close = __ipc_client_close,
};

/* ============================================================================
 *  PUBLIC Global Context methods
 */
int raleigh_initialize (void) {
  /* Initialize allocator */
  if (z_system_allocator_open(&(__global_ctx.allocator)))
    return(1);

  /* Initialize global context */
  __global_ctx.is_running = 1;
  if (z_global_context_open(&(__global_ctx.allocator), &__global_ctx)) {
    z_allocator_close(&(__global_ctx.allocator));
    return(1);
  }

  /* Initialize I/O Poll */
  if (z_iopoll_open(&(__global_ctx.iopoll), NULL, 1)) {
    fprintf(stderr, "z_iopoll_open(): failed\n");
    z_global_context_close();
    z_allocator_close(&(__global_ctx.allocator));
    return(1);
  }

  z_iopoll_poll(&(__global_ctx.iopoll), 1, &(__global_ctx.is_running), 1000);

  return(0);
}

void raleigh_uninitialize (void) {
  __global_ctx.is_running = 0;
  z_iopoll_wait(&(__global_ctx.iopoll));
  z_iopoll_close(&(__global_ctx.iopoll));
  z_global_context_close();
  z_allocator_close(&(__global_ctx.allocator));
}

/* ============================================================================
 *  PUBLIC Client methods
 */
raleigh_client_t *raleigh_client_alloc (void) {
  raleigh_client_t *self;

  self = z_memory_struct_alloc(z_global_memory(), raleigh_client_t);
  if (Z_MALLOC_IS_NULL(self))
    return(NULL);

  z_ipc_msgbuf_open(&(self->msgbuf), 4096);

  if (z_rpc_map_open(&(self->rpc_map))) {
    z_memory_struct_free(z_global_memory(), raleigh_client_t, self);
    return(NULL);
  }

  self->req_id = 0;
  return(self);
}

void raleigh_client_free (raleigh_client_t *self) {
  z_rpc_map_close(&(self->rpc_map));
  z_ipc_msgbuf_close(&(self->msgbuf));
  z_memory_struct_free(z_global_memory(), raleigh_client_t, self);
}

raleigh_client_t *raleigh_tcp_connect (const char *address, const char *port, int async) {
  raleigh_client_t *self;
  int sock;

  if ((sock = z_socket_tcp_connect(address, port, NULL)) < 0)
    return(NULL);

  self = raleigh_client_alloc();
  if (Z_MALLOC_IS_NULL(self)) {
    close(sock);
    return(NULL);
  }

  z_iopoll_entity_open(Z_IOPOLL_ENTITY(self), &__ipc_client_vtable, sock);
  if (async || __RALEIGH_USE_ASYNC_AND_LATCH) {
    z_iopoll_add(&(__global_ctx.iopoll), Z_IOPOLL_ENTITY(self));
  }
  return(self);
}
