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
#include <zcl/iopoll.h>
#include <zcl/debug.h>

#include <signal.h>
#include <stdio.h>

#include "server.h"

static struct server_context __global_ctx;

static void __signal_handler (int signum) {
  __global_ctx.is_running = 0;
}

static int __raleighsl_open (void) {
  raleighsl_t *fs = &(__global_ctx.fs);
  raleighsl_errno_t errno;

  if (raleighsl_alloc(fs) == NULL) {
    Z_LOG_FATAL("raleighsl: allocatin failed\n");
    return(1);
  }

  /* Plug objects */
  raleighsl_plug_object(fs, &raleighsl_object_number);
  raleighsl_plug_object(fs, &raleighsl_object_deque);
  raleighsl_plug_object(fs, &raleighsl_object_sset);
  raleighsl_plug_object(fs, &raleighsl_object_flow);

  /* TODO */
  const raleighsl_semantic_plug_t *semantic = &raleighsl_semantic_flat;
  const raleighsl_format_plug_t *format = NULL;
  const raleighsl_space_plug_t *space = NULL;
  raleighsl_device_t *device = NULL;

  if ((errno = raleighsl_create(fs, device, format, space, semantic))) {
    Z_LOG_FATAL("raleighsl: %s\n", raleighsl_errno_string(errno));
    raleighsl_free(fs);
    return(2);
  }

  return(0);
}

static void __raleighsl_close (void) {
  raleighsl_close(&(__global_ctx.fs));
  raleighsl_free(&(__global_ctx.fs));
}

static void __unplug_ipc (z_ipc_server_t *servers[], int n) {
  while (n--) {
    z_ipc_unplug(&(__global_ctx.iopoll), servers[n]);
  }
}

int main (int argc, char **argv) {
  z_ipc_server_t *tcp_server[4];
#ifdef Z_SOCKET_HAS_UNIX
  //z_ipc_server_t *unix_server[1];
#endif /* Z_SOCKET_HAS_UNIX */

  /* Initialize signals */
  signal(SIGINT, __signal_handler);

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
  if (z_iopoll_open(&(__global_ctx.iopoll), NULL, 0)) {
    Z_LOG_FATAL("z_iopoll_open(): failed\n");
    z_global_context_close();
    z_allocator_close(&(__global_ctx.allocator));
    return(1);
  }

  /* Initialize RaleighSL */
  if (__raleighsl_open()) {
    z_iopoll_close(&(__global_ctx.iopoll));
    z_global_context_close();
    z_allocator_close(&(__global_ctx.allocator));
    return(1);
  }

  /* Setup RPC plugins */
  tcp_server[0] = z_raleighsl_tcp_plug(&(__global_ctx.iopoll), NULL, "11215", &__global_ctx);
  tcp_server[1] = z_echo_tcp_plug(&(__global_ctx.iopoll),  NULL, "11212", &__global_ctx);
  tcp_server[2] = z_redis_tcp_plug(&(__global_ctx.iopoll), NULL, "11213", &__global_ctx);
  tcp_server[3] = z_stats_tcp_plug(&(__global_ctx.iopoll), NULL, "11217", &__global_ctx);

#ifdef Z_SOCKET_HAS_UNIX
  //unix_server[0] = z_raleighsl_unix_plug(&(__global_ctx.iopoll), "raleighsl.sock", &__global_ctx);
#endif /* Z_SOCKET_HAS_UNIX */

  /* Start spinning... */
  z_iopoll_poll(&(__global_ctx.iopoll), 0, &(__global_ctx.is_running), 2000);

  /* ...and we're done */
  __unplug_ipc(tcp_server, 4);
#ifdef Z_SOCKET_HAS_UNIX
  //__unplug_ipc(unix_server, 1);
#endif /* Z_SOCKET_HAS_UNIX */

  __raleighsl_close();
  z_iopoll_close(&(__global_ctx.iopoll));
  z_global_context_close();
  z_allocator_close(&(__global_ctx.allocator));
  return(0);
}
