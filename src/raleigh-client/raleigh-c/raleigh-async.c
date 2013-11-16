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

#include <zcl/debug.h>
#include <zcl/time.h>
#include <zcl/rpc.h>

#include "generated/rpc_client.h"
#include "raleigh.h"
#include "private.h"

/* ============================================================================
 *  Async Ping
 */
static void __raleigh_ping_callback (z_rpc_ctx_t *ctx, void *ucallback, void *udata) {
  __raleigh_call(raleigh_ping_t, ucallback, udata);
}

int raleigh_ping_async (raleigh_client_t *self, raleigh_ping_t callback, void *udata) {
  z_rpc_ctx_t *ctx;

  ctx = raleighsl_rpc_client_build_request(Z_IOPOLL_ENTITY(self), SERVER_PING_ID, self->req_id++);
  if (Z_UNLIKELY(ctx == NULL))
    return(-1);

  /* TODO: Fill the request */

  if (__raleigh_client_push_request(self, ctx, __raleigh_ping_callback, callback, udata)) {
    z_rpc_ctx_free(ctx);
    return(-2);
  }

  return(0);
}
