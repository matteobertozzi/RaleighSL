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

#include <zcl/iopoll.h>
#include <zcl/debug.h>
#include <zcl/time.h>

#include <signal.h>

#include "server.h"

static volatile int __running = 1;
static void __signal_term_handler (int signum) {
  __running = 0;
}

static void __signal_conf_handler (int signum) {
  /* TODO: refresh conf */
}

static int __server_open (r5l_server_t *self) {
  z_sys_allocator_init(&(self->allocator));
  z_memory_open(&(self->memory), &(self->allocator));

  z_ipc_msg_stats_init(&(self->ipc_msg_req_stats));
  z_ipc_msg_stats_init(&(self->ipc_msg_resp_stats));

  z_iopoll_engine_open(&(self->iopoll));
  echo_tcp_plug(self, "127.0.0.1", "11124");
  metrics_tcp_plug(self, "127.0.0.1", "11125");
  msgtest_tcp_plug(self, "127.0.0.1", "11126");

  return(0);
}

static void __server_close (r5l_server_t *self) {
  echo_tcp_unplug(self);
  metrics_tcp_unplug(self);
  msgtest_tcp_unplug(self);

  z_iopoll_engine_close(&(self->iopoll));
  z_iopoll_engine_dump(&(self->iopoll), stdout);

  z_ipc_msg_stats_dump(&(self->ipc_msg_req_stats), "Req", stderr);
  z_ipc_msg_stats_dump(&(self->ipc_msg_resp_stats), "Resp", stderr);

  z_memory_stats_dump(&(self->memory), stderr);
  z_memory_close(&(self->memory));
}

int main (int argc, char **argv) {
  r5l_server_t server;

  z_debug_open();

  /* Initialize signals */
  signal(SIGINT,  __signal_term_handler);
  signal(SIGTERM, __signal_term_handler);
  signal(SIGHUP,  __signal_conf_handler);

  __server_open(&server);
  while (__running) {
    z_iopoll_engine_poll(&(server.iopoll));
  }
  z_iopoll_engine_poll(&(server.iopoll));
  __server_close(&server);

  z_debug_close();
  return(0);
}
