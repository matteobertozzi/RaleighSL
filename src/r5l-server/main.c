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

#include <zcl/version.h>
#include <zcl/global.h>
#include <zcl/debug.h>
#include <zcl/time.h>

#include <string.h>
#include <signal.h>
#include <stdio.h>

#include "server.h"

struct signal_data {
  int term_signum;
};

static struct signal_data __signal_data;
static void __signal_handler (int signum) {
  __signal_data.term_signum = signum;
  z_global_context_stop();
}

static int __global_ctx_open (uint32_t ncores) {
  z_global_conf_t conf;

  /* Default Settings */
  memset(&conf, 0, sizeof(z_global_conf_t));
  conf.ncores = ncores;

  /* Memory Pool */
  conf.mmpool_base_size = (1 << 20);
  conf.mmpool_page_size = (1 << 20);
  conf.mmpool_block_min = (1 << 4);
  conf.mmpoll_block_max = (1 << 9);

  /* Events */
  conf.local_ring_size = 1024;
  conf.remote_ring_size = 256;

  /* User data */
  conf.udata_size = sizeof(struct server_context);

  return z_global_context_open(&conf);
}

static void __unplug_ipc (z_ipc_server_t *servers[], int n) {
  while (n--) {
    if (servers[n] != NULL) {
      z_ipc_unplug(servers[n]);
    }
  }
}

int main (int argc, char **argv) {
  fprintf(stdout, "%s %s (build-nr: %s git-rev: %s)\n",
                  R5L_SERVER_NAME, R5L_SERVER_VERSION_STR,
                  R5L_SERVER_BUILD_NR, R5L_SERVER_GIT_REV);
  //r5l_info_dump(stdout);
  zcl_info_dump(stdout);

  /* Initialize signals */
  __signal_data.term_signum = 0;
  signal(SIGINT,  __signal_handler);
  signal(SIGTERM, __signal_handler);

  /* Initialize global context */
  if (__global_ctx_open(1)) {
    Z_LOG_FATAL("z_global_context_open(): failed\n");
    return(1);
  }

  /* Setup RPC plugins */
  do {
    const long base_port = argc > 1 ? strtol(argv[1], NULL, 10) : 11212;
    char service[8];

    snprintf(service, 8, "%ld", base_port + 0);
    __global_server_context()->icp_server[ECHO_SERVER] = echo_tcp_plug(NULL, service);

    snprintf(service, 8, "%ld", base_port + 1);
    __global_server_context()->icp_server[METRICS_SERVER] = metrics_tcp_plug(NULL, service);

    snprintf(service, 8, "%ld", base_port + 2);
    __global_server_context()->icp_server[UDO_SERVER] = udo_udp_plug(NULL, service);

    snprintf(service, 8, "%ld", base_port + 3);
    __global_server_context()->icp_server[R5L_SERVER] = r5l_tcp_plug(NULL, service);
  } while (0);

  /* Start spinning... */
  z_global_context_poll(0);

  /* ...and we're done */
  __unplug_ipc(__global_server_context()->icp_server,
               z_fix_array_size(__global_server_context()->icp_server));

  z_global_context_close();
  fprintf(stderr, "terminated with signum %d\n", __signal_data.term_signum);
  return(0);
}
