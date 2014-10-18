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

#include <zcl/debug.h>
#include <zcl/time.h>
#include <zcl/ipc.h>
#include <zcl/fd.h>

#include "server.h"

static int __client_connected (z_ipc_client_t *client) {
  struct echo_client *echo = (struct echo_client *)client;
  Z_LOG_INFO("echo client connected %p", echo);
  echo->size = 0;
  echo->st = z_time_micros();
  return(0);
}

static int __client_disconnected (z_ipc_client_t *client) {
  struct echo_client *echo = (struct echo_client *)client;
  char buffer[64];
  float sec;

  Z_LOG_INFO("echo client disconnected %p", echo);
  sec = (z_time_micros() - echo->st) / 1000000.0f;
  z_human_dops(buffer, sizeof(buffer), echo->reads / sec);
  Z_DEBUG("READ %.3fsec %s/sec", sec, buffer);
  z_human_dops(buffer, sizeof(buffer), echo->writes / sec);
  Z_DEBUG("WRITES %.3fsec %s/sec", sec, buffer);
  return(0);
}

static int __client_read (z_ipc_client_t *client) {
  struct echo_client *echo = (struct echo_client *)client;
  ssize_t n;

  if (echo->size == sizeof(echo->buffer))
    return(0);

  n = z_fd_read(Z_IOPOLL_ENTITY_FD(client), echo->buffer, sizeof(echo->buffer));
  if (Z_UNLIKELY(n < 0))
    return(-1);

  echo->reads++;
  echo->size += n;
  z_ipc_client_set_data_available(client, echo->size);
  return(0);
}

static int __client_write (z_ipc_client_t *client) {
  struct echo_client *echo = (struct echo_client *)client;
  ssize_t n;

#if 0
  n = z_fd_write(Z_IOPOLL_ENTITY_FD(client), echo->buffer, echo->size);
  if (Z_UNLIKELY(n < 0))
    return(-1);

  echo->writes++;
  echo->size -= n;
#else
  n = z_fd_write(Z_IOPOLL_ENTITY_FD(client), "+PONG\r\n", 7);
  if (Z_UNLIKELY(n < 0))
    return(-1);

  echo->writes++;
  echo->size = 0;
#endif
  z_ipc_client_set_writable(client, echo->size);
  return(0);
}

const z_ipc_protocol_t echo_tcp_protocol = {
  /* raw-client protocol */
  .read         = __client_read,
  .write        = __client_write,
  /* client protocol */
  .connected    = __client_connected,
  .disconnected = __client_disconnected,

  /* server protocol */
  .uevent       = NULL,
  .timeout      = NULL,
  .accept       = z_ipc_accept_tcp,
  .bind         = z_ipc_bind_tcp,
  .unbind       = NULL,
  .setup        = NULL,
};
