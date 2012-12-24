/*
 *   Copyright 2011-2012 Matteo Bertozzi
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

#include <signal.h>
#include <stdio.h>

#include "server.h"

static int __is_running = 1;
static void __signal_handler (int signum) {
    __is_running = 0;
}

int main (int argc, char **argv) {
  z_memory_t memory;
  z_iopoll_t iopoll;

  /* Initialize signals */
  signal(SIGINT, __signal_handler);

  /* Initialize memory */
  z_system_memory_init(&memory);

  printf("iopoll: %lu\n", sizeof(z_iopoll_t));
  printf("ipc server: %lu\n", sizeof(z_ipc_server_t));
  printf("ipc client: %lu\n", sizeof(z_ipc_client_t));

  /* Initialize I/O Poll */
  if (z_iopoll_open(&iopoll, NULL)) {
      fprintf(stderr, "z_iopoll_open(): failed\n");
      return(1);
  }

  /* Setup RPC plugins */
  z_ipc_echo_plug(&memory, &iopoll, NULL, "11214", NULL);

  /* Start spinning... */
  z_iopoll_poll(&iopoll, &__is_running, -1);

  /* ...and we're done */
  z_iopoll_close(&iopoll);
  return(0);
}
