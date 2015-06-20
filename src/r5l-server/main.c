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
#include <zcl/debug.h>
#include <zcl/eloop.h>
#include <zcl/time.h>
#include <zcl/ipc.h>

#include <r5l/version.h>

#include <string.h>
#include <signal.h>
#include <stdio.h>

#include "server.h"

struct signal_data {
  int term_signum;
};

static z_event_loop_t __eloop[1];

static struct signal_data __signal_data;
static void __signal_handler (int signum) {
  __signal_data.term_signum = signum;
  z_event_loop_stop(__eloop + 0);
}

static void __dump_info_header (void) {
  fprintf(stdout, "%s %s (build-nr: %s git-rev: %s)\n",
                  R5L_SERVER_NAME, R5L_SERVER_VERSION_STR,
                  R5L_SERVER_BUILD_NR, R5L_SERVER_GIT_REV);
  r5l_info_dump(stdout);
  zcl_info_dump(stdout);
}

int main (int argc, char **argv) {
  __dump_info_header();

  /* Initialize signals */
  __signal_data.term_signum = 0;
  signal(SIGINT,  __signal_handler);
  signal(SIGTERM, __signal_handler);

  z_debug_open();
  z_event_loop_open(__eloop + 0, 0);
  z_event_loop_start(__eloop + 0, 1);

  z_ipc_plug(&(__eloop[0].iopoll), &(__eloop[0].memory), &udo_udp_protocol, NULL,
             "udo", struct udo_client, NULL, "11123");
  z_ipc_plug(&(__eloop[0].iopoll), &(__eloop[0].memory), &echo_tcp_protocol, NULL,
             "echo", struct echo_client, NULL, "11124");
  z_event_loop_wait(__eloop + 0);

  z_event_loop_close(__eloop + 0);
  z_event_loop_dump(__eloop + 0, stderr);
  z_debug_close();

  fprintf(stderr, "terminated with signum %d\n", __signal_data.term_signum);
  return(0);
}
