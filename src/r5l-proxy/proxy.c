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

#include "proxy.h"

r5l_proxy_t *r5l_proxy_open (void) {
  r5l_proxy_t *proxy;
  proxy = (r5l_proxy_t *) z_sys_alloc(sizeof(r5l_proxy_t));
  if (Z_MALLOC_IS_NULL(proxy))
    return(NULL);

  proxy->term_signum = 0;

  z_event_loop_open(proxy->eloop + 0, NULL, 1);

  return(proxy);
}

void r5l_proxy_close (r5l_proxy_t *proxy) {
  z_event_loop_close(proxy->eloop + 0);
  z_event_loop_dump(proxy->eloop + 0, stderr);

  fprintf(stderr, "terminated with signum %d\n", proxy->term_signum);
  z_sys_free(proxy, sizeof(r5l_proxy_t));
}

void r5l_proxy_stop_signal (r5l_proxy_t *proxy, int signum) {
  proxy->term_signum = signum;
  z_event_loop_stop(proxy->eloop + 0);
}