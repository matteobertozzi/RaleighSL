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
#include <zcl/time.h>

#include "proxy.h"

static int __client_msg_alloc (void *entity, z_msg_ibuf_t *ibuf,
                               uint32_t frame_length)
{
  r5l_proxy_client_t *client = R5L_PROXY_CLIENT(entity);
  r5l_proxy_t *server;

  server = z_ipc_get_server(r5l_proxy_t, entity);
  (void)server;

  printf("alloc msg for client %p frame-length %u\n", client, frame_length);
  //malloc(frame_length);
  return(0);
}

static int __client_msg_append (void *entity, z_msg_ibuf_t *ibuf,
                                const uint8_t *buffer, uint32_t size)
{
  r5l_proxy_client_t *client = R5L_PROXY_CLIENT(entity);
  printf("append to msg for client %p size %u\n", client, size);
  return(0);
}

static int __client_msg_publish (void *entity, z_msg_ibuf_t *ibuf) {
  r5l_proxy_client_t *client = R5L_PROXY_CLIENT(entity);
  r5l_proxy_t *server;

  server = z_ipc_get_server(r5l_proxy_t, entity);
  (void)server;

  printf("publish message for client %p\n", client);
  return(0);
}

const z_msg_protocol_t r5l_proxy_msg_protocol = {
  .alloc    = __client_msg_alloc,
  .append   = __client_msg_append,
  .publish  = __client_msg_publish,
};
