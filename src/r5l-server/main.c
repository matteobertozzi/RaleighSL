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

#include <string.h>
#include <signal.h>
#include <stdio.h>

#include "server.h"

static int __running = 1;
static void __signal_term_handler (int signum) {
  __running = 0;
}

static void __signal_conf_handler (int signum) {
  /* TODO: refresh conf */
}

static void *__event_loop_mmalloc (z_allocator_t *self, size_t size) {
  void *ptr;
  ptr = z_sys_alloc(size);
  if (Z_LIKELY(ptr != NULL)) {
    self->extern_alloc++;
    self->extern_used += size;
  }
  return(ptr);
}

static void __event_loop_mmfree (z_allocator_t *self, void *ptr, size_t size) {
  self->extern_used -= size;
  Z_LOG_TRACE("pool_used=%"PRIu64" extern_used=%"PRIu64,
              self->pool_used, self->extern_used);
  z_sys_free(ptr, size);
}

static void __eloop_open (struct eloop *self) {
  self->allocator.mmalloc = __event_loop_mmalloc;
  self->allocator.mmfree  = __event_loop_mmfree;
  self->allocator.pool_alloc = 0;
  self->allocator.pool_used  = 0;
  self->allocator.extern_alloc = 0;
  self->allocator.extern_used  = 0;
  z_memory_open(&(self->memory), &(self->allocator));
  z_iopoll_engine_open(&(self->iopoll));
  z_vtask_queue_init(&(self->rq));
  z_ipc_msg_stats_init(&(self->ipc_msg_req_stats));
  z_ipc_msg_stats_init(&(self->ipc_msg_resp_stats));
}

static void __eloop_close (struct eloop *self) {
  z_iopoll_engine_dump(&(self->iopoll), stderr);
  z_iopoll_engine_close(&(self->iopoll));

  z_ipc_msg_stats_dump(&(self->ipc_msg_req_stats), "Req", stderr);
  z_ipc_msg_stats_dump(&(self->ipc_msg_resp_stats), "Resp", stderr);

  z_memory_stats_dump(&(self->memory), stderr);
  z_memory_close(&(self->memory));
}

int main (int argc, char **argv) {
  z_ipc_server_t *server[2];
  struct eloop eloop;

  z_debug_open();

  /* Initialize signals */
  signal(SIGINT,  __signal_term_handler);
  signal(SIGTERM, __signal_term_handler);
  signal(SIGHUP,  __signal_conf_handler);

  /* Initialize eloop */
  __eloop_open(&eloop);

  /* Initialize IPCs */
  server[0] = echo_tcp_plug(&eloop, "127.0.0.1", "11124", &eloop);
  server[1] = metrics_tcp_plug(&eloop, "127.0.0.1", "11125", &eloop);

  /* Event Loop */
  while (__running) {
    z_iopoll_engine_poll(&(eloop.iopoll));
    while (eloop.rq.head != NULL) {
      z_vtask_t *vtask = z_vtask_queue_pop(&(eloop.rq));
      z_vtask_exec(vtask);
    }
  }

  /* Shutdown */
  z_ipc_unplugs(server, 2);
  __eloop_close(&eloop);

  z_debug_close();
  return(0);
}
