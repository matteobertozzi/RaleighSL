
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

#include <zcl/version.h>
#include <zcl/debug.h>
#include <zcl/time.h>

#include <string.h>
#include <signal.h>
#include <stdio.h>

#include "proxy.h"

static r5l_proxy_t *__proxy;
static void __signal_handler (int signum) {
  r5l_proxy_stop_signal(__proxy, signum);
}

static void __dump_info_header (FILE *stream) {
  fprintf(stream, "%s %s (build-nr: %s git-rev: %s)\n",
                  R5L_PROXY_NAME, R5L_PROXY_VERSION_STR,
                  R5L_PROXY_BUILD_NR, R5L_PROXY_GIT_REV);
  zcl_info_dump(stream);
}

int main (int argc, char **argv) {
  z_ipc_server_t *server[4];

  __dump_info_header(stdout);

  z_debug_open();
  if ((__proxy = r5l_proxy_open()) == NULL)
    return(1);

  /* Initialize signals */
  signal(SIGINT,  __signal_handler);
  signal(SIGTERM, __signal_handler);

  z_tls_lib_open();

  server[0] = r5l_proxy_udp_plug(__proxy->eloop + 0, "127.0.0.1", "15701", NULL);
  server[1] = r5l_proxy_tcp_plug(__proxy->eloop + 0, "127.0.0.1", "15702", NULL);
  server[2] = r5l_proxy_tls_plug(__proxy->eloop + 0, "127.0.0.1", "15703", NULL);
  server[3] = r5l_proxy_unix_plug(__proxy->eloop + 0, "r5l-proxy.sock", NULL);

  z_event_loop_start(__proxy->eloop + 0, 1);
  z_event_loop_wait(__proxy->eloop + 0);

  z_ipc_unplugs(server, 4);
  z_tls_lib_close();

  r5l_proxy_close(__proxy);
  z_debug_close();
  return(0);
}
