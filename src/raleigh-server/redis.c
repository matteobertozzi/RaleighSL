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

#include <zcl/humans.h>
#include <zcl/strtol.h>
#include <zcl/slice.h>
#include <zcl/debug.h>
#include <zcl/iovec.h>
#include <zcl/ipc.h>

#include <unistd.h>
#include <stdio.h>

#include "server.h"

#define MAX_TOKENS          16
#define COMMAND_TOKEN       0

static int __process_ping (struct redis_client *client,
                           const z_slice_t *tokens,
                           size_t ntokens)
{
  return(z_ringbuf_push(&(client->obuffer), "+PONG\r\n", 7));
}

static int __process_quit (struct redis_client *client,
                           const z_slice_t *tokens,
                           size_t ntokens)
{
  z_ringbuf_push(&(client->obuffer), "+OK\r\n", 5);
  return(-1);
}

/* ===========================================================================
 *  Redis protocol
 */
typedef int (*redis_func_t) (struct redis_client *client,
                             const z_slice_t *tokens,
                             size_t ntokens);

struct redis_command {
  const char * name;
  unsigned int length;
  redis_func_t func;
};

static const struct redis_command __redis_commands[] = {
  { "ping", 4, __process_ping },
  { "quit", 4, __process_quit },
  { NULL, 0, NULL },
};

static int __redis_process_command(struct redis_client *client,
                                   z_slice_t *tokens,
                                   size_t ntokens)
{
  const struct redis_command *p;

  z_slice_to_lower(&(tokens[COMMAND_TOKEN]));
  for (p = __redis_commands; p->name != NULL; ++p) {
    if (z_slice_length(&(tokens[COMMAND_TOKEN])) < p->length) continue;
    if (!z_slice_equals(&(tokens[COMMAND_TOKEN]), p->name, p->length))
      continue;

    return(p->func(client, tokens, ntokens));
  }

  return(-1);
}


static int __redis_process_multibulk (struct redis_client *client,
                                      z_iovec_reader_t *reader,
                                      z_slice_t *tokens,
                                      size_t *trim)
{
  char buffer[16];
  size_t ntokens;
  uint32_t nargs;
  ssize_t n;

  /* Remove the '*' */
  z_slice_ltrim(&(tokens[0]), 1);
  n = z_slice_copy(&(tokens[0]), buffer, sizeof(buffer));
  buffer[n] = '\0';

  if (!z_strtou32(buffer, 10, &nargs)) {
    return(-1);
  }

  if (nargs > MAX_TOKENS) {
    return(-1);
  }

  ntokens = 0;
  while (nargs--) {
    uint32_t size;

    /* Read length */
    while ((n = z_reader_tokenize(reader, " \t\r\n\v\f", 6, &(tokens[ntokens]))) > 0) {
      if (!z_slice_is_empty(&(tokens[ntokens])))
        break;
      *trim += n;
    }

    /* TODO: Data not ready */
    if (n <= 0) {
      *trim = 0;
      return(1);
    }

    *trim += n;
    if (!z_slice_starts_with(&(tokens[ntokens]), "$", 1)) {
      return(-1);
    }

    z_slice_ltrim(&(tokens[ntokens]), 1);
    n = z_slice_copy(&(tokens[ntokens]), buffer, sizeof(buffer));
    buffer[n] = '\0';
    if (!z_strtou32(buffer, 10, &size)) {
      return(-1);
    }

    /* TODO: Read Value using size */
    while ((n = z_reader_tokenize(reader, " \t\r\n\v\f", 6, &(tokens[ntokens]))) > 0) {
      if (!z_slice_is_empty(&(tokens[ntokens])))
        break;
      *trim += n;
    }

    /* TODO: Data not ready */
    if (n <= 0) {
      *trim = 0;
      return(1);
    }

    *trim += n;
    ++ntokens;
  }

  return(__redis_process_command(client, tokens, ntokens));
}

static int __redis_process_inline (struct redis_client *client,
                                   z_iovec_reader_t *reader,
                                   z_slice_t *tokens,
                                   size_t *trim)
{
  size_t ntokens;
  ssize_t n;

  /* TODO: Try to lookup \n */
  ntokens = 1;
  while ((n = z_reader_tokenize(reader, " \t\r\n\v\f", 6, &(tokens[ntokens]))) > 0) {
    (*trim) += n;
    if (z_slice_is_empty(&(tokens[ntokens])))
      continue;

    ++ntokens;
  }

  return(__redis_process_command(client, tokens, ntokens));
}

/* ===========================================================================
 *  IPC protocol handlers
 */
static int __client_connected (z_ipc_client_t *client) {
  struct redis_client *redis = (struct redis_client *)client;
  z_ringbuf_alloc(&(redis->ibuffer), 512);
  z_ringbuf_alloc(&(redis->obuffer), 512);
  Z_LOG_INFO("Redis Client %d connected", Z_IOPOLL_ENTITY_FD(client));
  return(0);
}

static void __client_disconnected (z_ipc_client_t *client) {
  struct redis_client *redis = (struct redis_client *)client;
  z_ringbuf_free(&(redis->ibuffer));
  z_ringbuf_free(&(redis->obuffer));
  Z_LOG_INFO("Redis Client %d disconnected", Z_IOPOLL_ENTITY_FD(client));
}

static int __client_read (z_ipc_client_t *ipc_client) {
  struct redis_client *client = (struct redis_client *)ipc_client;
  z_slice_t tokens[MAX_TOKENS];
  z_iovec_reader_t reader;
  struct iovec iov[2];
  int result = 1;
  size_t trim;
  ssize_t n;
  int i;

  if (z_ringbuf_is_full(&(client->ibuffer)))
    return(0);

  if ((n = z_ringbuf_fd_fetch(&(client->ibuffer), Z_IOPOLL_ENTITY_FD(client))) <= 0)
    return(-1);

  z_ringbuf_pop_iov(&(client->ibuffer), iov, client->ibuffer.size);
  z_iovec_reader_open(&reader, iov, 2);

  for (i = 0; i < MAX_TOKENS; ++i)
    z_slice_alloc(&(tokens[i]));

  trim = 0;
  while ((n = z_reader_tokenize(&reader, " \t\r\n\v\f", 6, &(tokens[0]))) > 0) {
    if (!z_slice_is_empty(&(tokens[0])))
      break;
    trim += n;
  }

  if (n > 0) {
    trim += n;
    if (z_slice_starts_with(&(tokens[0]), "*", 1))
      result = __redis_process_multibulk(client, &reader, tokens, &trim);
    else
      result = __redis_process_inline(client, &reader, tokens, &trim);
  }

  for (i = 0; i < MAX_TOKENS; ++i)
    z_slice_free(&(tokens[i]));
  z_iovec_reader_close(&reader);
  z_ringbuf_rskip(&(client->ibuffer), trim);
  z_ipc_client_set_writable(client, z_ringbuf_has_data(&(client->obuffer)));
  return(result);
}

static int __client_write (z_ipc_client_t *client) {
  struct redis_client *redis = (struct redis_client *)client;
  ssize_t n;

  if ((n = z_ringbuf_fd_dump(&(redis->obuffer), Z_IOPOLL_ENTITY_FD(client))) <= 0)
    return(-1);

  z_ipc_client_set_writable(client, 0);
  return(0);
}

const struct z_ipc_protocol redis_protocol = {
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