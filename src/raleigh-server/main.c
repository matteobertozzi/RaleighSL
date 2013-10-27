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

#include <signal.h>
#include <stdio.h>

#include "server.h"

static struct server_context __global_ctx;

static void __signal_handler (int signum) {
  __global_ctx.is_running = 0;
}

static int __raleighfs_open (void) {
  raleighfs_t *fs = &(__global_ctx.fs);
  raleighfs_errno_t errno;

  if (raleighfs_alloc(fs) == NULL) {
    fprintf(stderr, "raleighfs: allocatin failed\n");
    return(1);
  }

  /* Plug objects */
  raleighfs_plug_object(fs, &raleighfs_object_counter);
  raleighfs_plug_object(fs, &raleighfs_object_deque);
  raleighfs_plug_object(fs, &raleighfs_object_sset);

  /* TODO */
  const raleighfs_semantic_plug_t *semantic = &raleighfs_semantic_flat;
  const raleighfs_format_plug_t *format = NULL;
  const raleighfs_space_plug_t *space = NULL;
  raleighfs_device_t *device = NULL;

  if ((errno = raleighfs_create(fs, device, format, space, semantic))) {
    fprintf(stderr, "raleighfs: %s\n", raleighfs_errno_string(errno));
    raleighfs_free(fs);
    return(2);
  }

  return(0);
}

static void __raleighfs_close (void) {
  raleighfs_close(&(__global_ctx.fs));
  raleighfs_free(&(__global_ctx.fs));
}

int main (int argc, char **argv) {
  z_ipc_server_t *server[4];

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
  if (z_iopoll_open(&(__global_ctx.iopoll), NULL)) {
    fprintf(stderr, "z_iopoll_open(): failed\n");
    z_global_context_close();
    z_allocator_close(&(__global_ctx.allocator));
    return(1);
  }

  /* Initialize RaleighFS */
  if (__raleighfs_open()) {
    z_iopoll_close(&(__global_ctx.iopoll));
    z_global_context_close();
    z_allocator_close(&(__global_ctx.allocator));
    return(1);
  }

  /* Setup RPC plugins */
  server[0] = z_ipc_echo_plug(&(__global_ctx.iopoll),  NULL, "11212", &__global_ctx);
  server[1] = z_ipc_redis_plug(&(__global_ctx.iopoll), NULL, "11213", &__global_ctx);
  server[2] = z_ipc_stats_plug(&(__global_ctx.iopoll), NULL, "11217", &__global_ctx);
  server[3] = z_ipc_raleighfs_plug(&(__global_ctx.iopoll), NULL, "11215", &__global_ctx);

  /* Start spinning... */
  z_iopoll_poll(&(__global_ctx.iopoll), &(__global_ctx.is_running), 1000);

  /* ...and we're done */
  z_ipc_unplug(&(__global_ctx.iopoll), server[3]);
  z_ipc_unplug(&(__global_ctx.iopoll), server[2]);
  z_ipc_unplug(&(__global_ctx.iopoll), server[1]);
  z_ipc_unplug(&(__global_ctx.iopoll), server[0]);

  __raleighfs_close();
  z_iopoll_close(&(__global_ctx.iopoll));
  z_global_context_close();
  z_allocator_close(&(__global_ctx.allocator));
  return(0);
}
