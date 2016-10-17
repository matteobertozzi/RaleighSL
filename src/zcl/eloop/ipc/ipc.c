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

#include <zcl/debug.h>
#include <zcl/ipc.h>
#include <zcl/fd.h>

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

z_ipc_server_t *__z_ipc_get_server (const z_ipc_client_t *client) {
  return __ipc_server[client->io_entity.uflags8];
}

/* ============================================================================
 *  IPC client iopoll entity
 */
static const z_iopoll_entity_vtable_t __ipc_raw_client_vtable;
static const z_iopoll_entity_vtable_t __ipc_msg_client_vtable;

static z_ipc_client_t *__ipc_client_alloc (z_ipc_server_t *server, int csock) {
  z_ipc_client_t *client;

  /* Allocate client */
  client = z_memory_alloc(server->memory, z_ipc_client_t, server->csize);
  if (Z_MALLOC_IS_NULL(client))
    return(NULL);

  if (server->msg_protocol != NULL) {
    /* Initialize msg-client */
    z_ipc_msg_client_t *msg_client = Z_IPC_MSG_CLIENT(client);
    z_iopoll_entity_open(&(client->io_entity), &__ipc_msg_client_vtable, csock);
    z_ipc_msg_reader_open(&(msg_client->reader));
    z_ipc_msg_writer_open(&(msg_client->writer));
  } else {
    /* Initialize raw-client */
    z_iopoll_entity_open(&(client->io_entity), &__ipc_raw_client_vtable, csock);
  }
  client->io_entity.uflags8 = server->io_entity.uflags8;

  /* Ask the protocol to do its own stuff before starting up */
  if (Z_LIKELY(server->protocol->connected != NULL)) {
    if (Z_UNLIKELY(server->protocol->connected(client))) {
      z_memory_free(server->memory, z_ipc_client_t, client, server->csize);
      return(NULL);
    }
  }
  return(client);
}

static void __ipc_client_close (z_iopoll_engine_t *engine,
                                z_iopoll_entity_t *entity)
{
  z_ipc_server_t *server = __ipc_server[entity->uflags8];
/*
  if (server->msg_protocol != NULL) {
    if (z_atomic_dec(&(Z_IPC_MSG_CLIENT(entity)->refs)) > 0) {
      Z_LOG_TRACE("client %d disconnect-delayed", entity->fd);
      return;
    }
  }
*/
  /* Ask the protocol to do its own stuff before closing down */
  if (Z_LIKELY(server->protocol->disconnected != NULL)) {
    if (Z_UNLIKELY(server->protocol->disconnected(Z_IPC_CLIENT(entity))))
      return;
  }

  /* Close client description */
  z_iopoll_entity_close(entity, 0);

  /* Uninitialize mgsbuf if the client is a msg-client */
  if (server->msg_protocol != NULL) {
    z_ipc_msg_client_t *msg_client = Z_IPC_MSG_CLIENT(entity);
    z_ipc_msg_reader_close(&(msg_client->reader));
    z_ipc_msg_writer_close(&(msg_client->writer));
  }

  /* Deallocate client */
  z_memory_set_dirty_debug(entity, server->csize);
  z_memory_free(server->memory, z_ipc_client_t, entity, server->csize);

  server->connections--;
}

/* ============================================================================
 *  IPC raw-client iopoll entity
 */
static int __ipc_raw_client_read (z_iopoll_engine_t *engine,
                                  z_iopoll_entity_t *entity)
{
  const z_ipc_protocol_t *proto = __ipc_server[entity->uflags8]->protocol;
  return(Z_LIKELY(proto->read) ? proto->read(Z_IPC_CLIENT(entity)) : -1);
}

static int __ipc_raw_client_write (z_iopoll_engine_t *iopoll,
                                   z_iopoll_entity_t *entity)
{
  const z_ipc_protocol_t *proto = __ipc_server[entity->uflags8]->protocol;
  return(Z_LIKELY(proto->write) ? proto->write(iopoll, Z_IPC_CLIENT(entity)) : -1);
}

static const z_iopoll_entity_vtable_t __ipc_raw_client_vtable = {
  .read     = __ipc_raw_client_read,
  .write    = __ipc_raw_client_write,
  .uevent   = NULL,
  .timeout  = NULL,
  .close    = __ipc_client_close,
};

/* ============================================================================
 *  IPC msg-client iopoll entity
 */
static int __ipc_msg_client_read (z_iopoll_engine_t *engine,
                                  z_iopoll_entity_t *entity)
{
  return(z_ipc_msg_reader_read(&(Z_IPC_MSG_CLIENT(entity)->reader),
                               &z_iopoll_entity_raw_io_seq_vtable,
                               __ipc_server[entity->uflags8]->msg_protocol,
                               entity));
}

static int __ipc_msg_client_write (z_iopoll_engine_t *engine,
                                   z_iopoll_entity_t *entity)
{
  const z_ipc_server_t *server = __ipc_server[entity->uflags8];
  return(z_ipc_msg_writer_flush(&(Z_IPC_MSG_CLIENT(entity)->writer),
                                server->memory,
                                &z_iopoll_entity_raw_io_seq_vtable,
                                server->msg_protocol,
                                entity));
}

static const z_iopoll_entity_vtable_t __ipc_msg_client_vtable = {
  .read  = __ipc_msg_client_read,
  .write = __ipc_msg_client_write,
  .uevent   = NULL,
  .timeout  = NULL,
  .close = __ipc_client_close,
};

/* ============================================================================
 *  IPC Server Private Methods
 */
static const z_iopoll_entity_vtable_t __ipc_server_vtable;

static int __ipc_server_bind (z_ipc_server_t *server,
                              const void *hostname,
                              const void *service)
{
  int socket;

  if ((socket = server->protocol->bind(server, hostname, service)) < 0) {
    return(-1);
  }

  if (z_fd_set_blocking(socket, 0)) {
    close(socket);
    return(-2);
  }

  z_iopoll_entity_open(&(server->io_entity), &__ipc_server_vtable, socket);
  server->io_entity.uflags8 = __ipc_server_register(server);

  z_iopoll_timer_open(&(server->timer), &__ipc_server_vtable, socket);
  z_iopoll_uevent_open(&(server->event), &__ipc_server_vtable, socket);

  if (server->protocol->setup != NULL && server->protocol->setup(server)) {
    close(socket);
    return(-3);
  }
  return(0);
}

static void __ipc_server_close (z_iopoll_engine_t *engine,
                                z_iopoll_entity_t *entity)
{
  z_ipc_server_t *server = Z_IPC_SERVER(entity);

  if (server->protocol->unbind != NULL)
    server->protocol->unbind(server);

  z_iopoll_entity_close(entity, 0);
}

static int __ipc_server_accept (z_iopoll_engine_t *engine,
                                z_iopoll_entity_t *entity)
{
  z_ipc_server_t *server = Z_IPC_SERVER(entity);
  int csock;

  csock = server->protocol->accept(server);
  if (Z_UNLIKELY(csock <= 0))
    return csock;

  return z_ipc_server_add(server, csock);
}

static int __ipc_server_uevent (z_iopoll_engine_t *engine,
                                z_iopoll_entity_t *entity)
{
  z_ipc_server_t *server = z_container_of(entity, z_ipc_server_t, event);
  return server->protocol->uevent(server);
}

static int __ipc_server_timeout (z_iopoll_engine_t *engine,
                                 z_iopoll_entity_t *entity)
{
  z_ipc_server_t *server = z_container_of(entity, z_ipc_server_t, timer);
  return server->protocol->timeout(server);
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
z_ipc_server_t *__z_ipc_plug (z_ipc_server_t *server,
                              z_iopoll_engine_t *engine,
                              z_memory_t *memory,
                              const z_ipc_protocol_t *proto,
                              const z_ipc_msg_protocol_t *msg_proto,
                              const char *name,
                              unsigned int csize,
                              const void *address,
                              const void *service,
                              void *udata)
{
  server->csize = csize;
  server->__pad0 = 0;
  server->connections = 0;
  server->udata.ptr = udata;

  server->protocol = proto;
  server->msg_protocol = msg_proto;
  server->engine = engine;
  server->memory = memory;

  server->name = name;

  if (__ipc_server_bind(server, address, service)) {
    return(NULL);
  }

  if (z_iopoll_engine_add(server->engine, &(server->io_entity))) {
    __ipc_server_close(server->engine, &(server->io_entity));
    return(NULL);
  }

  return(server);
}

void z_ipc_unplug (z_ipc_server_t *server) {
  z_iopoll_entity_t *entity = &(server->io_entity);
  z_iopoll_engine_remove(server->engine, entity);
  __ipc_server_close(server->engine, entity);
}

int z_ipc_server_add (z_ipc_server_t *server, int csock) {
  z_ipc_client_t *client;

  client = __ipc_client_alloc(server, csock);
  if (Z_UNLIKELY(client == NULL)) {
    close(csock);
    return(-1);
  }

  if (Z_UNLIKELY(z_iopoll_engine_add(server->engine, &(client->io_entity)))) {
    __ipc_client_close(server->engine, &(client->io_entity));
    return(-2);
  }

  server->connections++;
  return(0);
}
