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

#include <zcl/allocator.h>

#include "server.h"

r5l_server_t *r5l_server_open (void) {
  r5l_server_t *server;
  server = (r5l_server_t *) z_sys_alloc(sizeof(r5l_server_t));
  if (Z_MALLOC_IS_NULL(server))
    return(NULL);

  server->term_signum = 0;

  z_event_loop_open(server->eloop + 0, NULL, 1);

  return(server);
}

void r5l_server_close (r5l_server_t *server) {
  z_event_loop_close(server->eloop + 0);
  z_event_loop_dump(server->eloop + 0, stderr);

  fprintf(stderr, "terminated with signum %d\n", server->term_signum);
  z_sys_free(server, sizeof(r5l_server_t));
}

void r5l_server_stop_signal (r5l_server_t *server, int signum) {
  server->term_signum = signum;
  z_event_loop_stop(server->eloop + 0);
}