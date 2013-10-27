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

#include <zcl/ipc.h>

#include <unistd.h>
#include <stdio.h>

#include "server.h"

static int __client_connected (z_ipc_client_t *client) {
  struct echo_client *echo = (struct echo_client *)client;

  printf("client connected\n");
  z_ringbuf_alloc(&(echo->buffer), 64);
  return(0);
}

static void __client_disconnected (z_ipc_client_t *client) {
  struct echo_client *echo = (struct echo_client *)client;

  printf("client disconnected\n");
  z_ringbuf_free(&(echo->buffer));
}

static int __client_read (z_ipc_client_t *client) {
  struct echo_client *echo = (struct echo_client *)client;
  ssize_t n;

  if (z_ringbuf_is_full(&(echo->buffer)))
    return(0);

  if ((n = z_ringbuf_fd_fetch(&(echo->buffer), Z_IOPOLL_ENTITY_FD(client))) <= 0)
    return(-1);

  z_ipc_client_set_writable(client, 1);
  return(0);
}

static int __client_write (z_ipc_client_t *client) {
  struct echo_client *echo = (struct echo_client *)client;
  ssize_t n;

  if ((n = z_ringbuf_fd_dump(&(echo->buffer), Z_IOPOLL_ENTITY_FD(client))) <= 0)
    return(-1);

  z_ipc_client_set_writable(client, 0);
  return(0);
}

const struct z_ipc_protocol echo_protocol = {
  /* server protocol */
  .bind         = z_ipc_bind_tcp,
  .accept       = z_ipc_accept_tcp,
  .setup        = NULL,

  /* client protocol */
  .connected    = __client_connected,
  .disconnected = __client_disconnected,
  .read         = __client_read,
  .write        = __client_write,
};
