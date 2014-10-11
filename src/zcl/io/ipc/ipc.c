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

#include <zcl/global.h>
#include <zcl/memory.h>
#include <zcl/debug.h>
#include <zcl/ipc.h>
#include <zcl/fd.h>

#include <unistd.h>

/* ============================================================================
 *  PRIVATE IPC Server registration
 */
static z_ipc_server_t *__ipc_server[] = {
  /*  0- 7 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  /*  8-15 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};

static uint8_t __ipc_server_register (z_ipc_server_t *server) {
  uint8_t i;
  for (i = 0; i < z_fix_array_size(__ipc_server); ++i) {
    if (__ipc_server[i] == server || __ipc_server[i] == NULL) {
      __ipc_server[i] = server;
      return(i);
    }
  }
  Z_LOG_FATAL("Too many ipc-servers registered: %d", i);
  return(0xff);
}

/* ============================================================================
 *  IPC client iopoll entity
 */
static z_ipc_client_t *__ipc_client_alloc (z_ipc_server_t *server, int csock) {
  const z_ipc_protocol_t *proto = server->protocol;
  z_ipc_client_t *client;

  /* Allocate client */
  client = z_memory_alloc(z_global_memory(), z_ipc_client_t, server->csize);
  if (Z_MALLOC_IS_NULL(client))
    return(NULL);

  /* Initialize client */
  z_iopoll_entity_open(Z_IOPOLL_ENTITY(client), server->client_vtable, csock);
  Z_IOPOLL_ENTITY(client)->uflags8 = Z_IOPOLL_ENTITY(server)->uflags8;

  /* Initialize mgsbuf if the client is a msg-client */
  if (proto->msg_exec != NULL) {
    z_ipc_msg_client_t *msg_client = Z_IPC_MSG_CLIENT(client);
    z_ipc_msgbuf_open(&(msg_client->msgbuf));
    msg_client->refs = 1;
    z_ticket_init(&(msg_client->lock));
  }

  /* Ask the protocol to do its own stuff before starting up */
  if (proto->connected != NULL) {
    if (proto->connected(client)) {
      z_memory_free(z_global_memory(), client);
      return(NULL);
    }
  }

  return(client);
}

static void __ipc_client_close (z_iopoll_entity_t *entity) {
  z_ipc_server_t *server = __ipc_server[entity->uflags8];
  const z_ipc_protocol_t *proto = server->protocol;

  if (proto->msg_parse != NULL) {
    if (z_atomic_dec(&(Z_IPC_MSG_CLIENT(entity)->refs)) > 0) {
      Z_LOG_TRACE("client %d disconnect-delayed", entity->fd);
      return;
    }
  }

  /* Ask the protocol to do its own stuff before closing down */
  if (proto->disconnected != NULL) {
    if (proto->disconnected(Z_IPC_CLIENT(entity)))
      return;
  }

  /* Close client description */
  z_iopoll_entity_close(entity, 0);

  /* Uninitialize mgsbuf if the client is a msg-client */
  if (proto->msg_parse != NULL) {
    z_ipc_msgbuf_close(Z_IPC_MSG_CLIENT(entity), proto->msg_free);
  }

  /* Deallocate client */
  z_memory_set_dirty_debug(entity, __ipc_server[entity->uflags8]->csize);
  z_memory_free(z_global_memory(), entity);

  z_atomic_dec(&(server->connections));
}

/* ============================================================================
 *  IPC raw-client iopoll entity
 */
static int __ipc_raw_client_read (z_iopoll_entity_t *entity) {
  const z_ipc_protocol_t *proto = __ipc_server[entity->uflags8]->protocol;
  return(Z_LIKELY(proto->read) ? proto->read(Z_IPC_CLIENT(entity)) : -1);
}

static int __ipc_raw_client_write (z_iopoll_entity_t *entity) {
  const z_ipc_protocol_t *proto = __ipc_server[entity->uflags8]->protocol;
  return(Z_LIKELY(proto->write) ? proto->write(Z_IPC_CLIENT(entity)) : -1);
}

static const z_iopoll_entity_vtable_t __ipc_raw_client_vtable = {
  .read  = __ipc_raw_client_read,
  .write = __ipc_raw_client_write,
  .close = __ipc_client_close,
};

/* ============================================================================
 *  IPC msg-client iopoll entity
 */
static int __ipc_msg_client_read (z_iopoll_entity_t *entity) {
  const z_ipc_protocol_t *proto = __ipc_server[entity->uflags8]->protocol;
  return(z_ipc_msg_client_fetch(Z_IPC_MSG_CLIENT(entity),
                                proto->msg_alloc,
                                proto->msg_parse,
                                proto->msg_exec));
}

static int __ipc_msg_client_write (z_iopoll_entity_t *entity) {
  return(z_ipc_msg_client_flush(Z_IPC_MSG_CLIENT(entity)));
}

static const z_iopoll_entity_vtable_t __ipc_msg_client_vtable = {
  .read  = __ipc_msg_client_read,
  .write = __ipc_msg_client_write,
  .close = __ipc_client_close,
};

/* ============================================================================
 *  IPC Server Private Methods
 */
static const z_iopoll_entity_vtable_t __ipc_server_vtable;

static z_ipc_server_t *__ipc_server_alloc (const z_ipc_protocol_t *proto,
                                           const void *hostname,
                                           const void *service)
{
  z_ipc_server_t *server;
  int socket;

  server = z_memory_struct_alloc(z_global_memory(), z_ipc_server_t);
  if (Z_MALLOC_IS_NULL(server))
    return(NULL);

  if ((socket = proto->bind(hostname, service)) < 0) {
    z_memory_struct_free(z_global_memory(), z_ipc_server_t, server);
    return(NULL);
  }

  if (z_fd_set_blocking(socket, 0)) {
    close(socket);
    z_memory_struct_free(z_global_memory(), z_ipc_server_t, server);
    return(NULL);
  }

  z_iopoll_entity_open(Z_IOPOLL_ENTITY(server), &__ipc_server_vtable, socket);
  Z_IOPOLL_ENTITY(server)->uflags8 = __ipc_server_register(server);
  server->protocol = proto;

  if (proto->msg_parse != NULL) {
    server->client_vtable = &__ipc_msg_client_vtable;
  } else {
    server->client_vtable = &__ipc_raw_client_vtable;
  }

  z_iopoll_timer_open(&(server->timer), &__ipc_server_vtable, socket);
  z_iopoll_uevent_open(&(server->event), &__ipc_server_vtable, socket);

  if (proto->setup != NULL && proto->setup(server)) {
    close(socket);
    z_memory_struct_free(z_global_memory(), z_ipc_server_t, server);
    return(NULL);
  }

  return(server);
}

static void __ipc_server_close (z_iopoll_entity_t *entity) {
  z_ipc_server_t *server = Z_IPC_SERVER(entity);

  if (server->protocol->unbind != NULL)
    server->protocol->unbind(server);

  z_iopoll_entity_close(entity, 0);
  z_memory_struct_free(z_global_memory(), z_ipc_server_t, server);
}

static int __ipc_server_accept (z_iopoll_entity_t *entity) {
  z_ipc_server_t *server = Z_IPC_SERVER(entity);
  const z_ipc_protocol_t *proto;
  int csock;

  proto = server->protocol;
  csock = proto->accept(server);
  if (Z_UNLIKELY(csock < 0))
    return(-1);

  return(z_ipc_server_add(server, csock));
}

static int __ipc_server_uevent (z_iopoll_entity_t *entity) {
  z_ipc_server_t *server = z_container_of(entity, z_ipc_server_t, event);
  return(server->protocol->uevent(server));
}

static int __ipc_server_timeout (z_iopoll_entity_t *entity) {
  z_ipc_server_t *server = z_container_of(entity, z_ipc_server_t, timer);
  return(server->protocol->timeout(server));
}

static const z_iopoll_entity_vtable_t __ipc_server_vtable = {
  .read    = __ipc_server_accept,
  .write   = NULL,
  .uevent  = __ipc_server_uevent,
  .timeout = __ipc_server_timeout,
  .close   = __ipc_server_close,
};

/* ============================================================================
 *  PUBLIC IPC methods
 */
z_ipc_server_t *__z_ipc_plug (const z_ipc_protocol_t *proto,
                              const char *name,
                              unsigned int csize,
                              const void *address,
                              const void *service)
{
  z_ipc_server_t *server;

  server = __ipc_server_alloc(proto, address, service);
  if (Z_MALLOC_IS_NULL(server))
    return(NULL);

  server->name = name;
  server->csize = csize;
  server->connections = 0;

  if (z_iopoll_add(Z_IOPOLL_ENTITY(server))) {
    __ipc_server_close(Z_IOPOLL_ENTITY(server));
    return(NULL);
  }

  Z_LOG_INFO("Service %s %d up and running on %s:%s...",
             name, Z_IOPOLL_ENTITY_FD(server), address, service);
  return(server);
}

void z_ipc_unplug (z_ipc_server_t *server) {
  z_iopoll_entity_t *entity = Z_IOPOLL_ENTITY(server);
  z_iopoll_remove(entity);
  __ipc_server_close(entity);
}

int z_ipc_server_add (z_ipc_server_t *server, int csock) {
  z_ipc_client_t *client;

  client = __ipc_client_alloc(server, csock);
  if (Z_UNLIKELY(client == NULL)) {
    close(csock);
    return(-1);
  }

  if (Z_UNLIKELY(z_iopoll_add(Z_IOPOLL_ENTITY(client)))) {
    __ipc_client_close(Z_IOPOLL_ENTITY(client));
    return(-2);
  }

  z_atomic_inc(&(server->connections));
  return(0);
}

z_ipc_server_t *z_ipc_get_server (const z_ipc_client_t *client) {
  return __ipc_server[Z_IOPOLL_ENTITY(client)->uflags8];
}

/* ============================================================================
 *  PUBLIC  IPC Msg-Client methods
 */
int z_ipc_msg_client_respond (z_ipc_msg_client_t *client, void *ctx) {
  const z_ipc_protocol_t *proto = __ipc_server[Z_IOPOLL_ENTITY(client)->uflags8]->protocol;
  if (z_atomic_dec(&(client->refs)) == 0) {
    proto->msg_free(client, ctx);
    __ipc_client_close(Z_IOPOLL_ENTITY(client));
    return(-1);
  }
  return(proto->msg_respond(ctx));
}
