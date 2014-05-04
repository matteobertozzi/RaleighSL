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

#include <zcl/rpc.h>

int z_rpc_map_open (z_rpc_map_t *self) {
  return(0);
}

void z_rpc_map_close (z_rpc_map_t *self) {
}

z_rpc_call_t *z_rpc_map_remove (z_rpc_map_t *self, uint64_t req_id) {
  return(NULL);
}

z_rpc_call_t *z_rpc_map_get (z_rpc_map_t *self, uint64_t req_id) {
  return(NULL);
}

z_rpc_call_t *z_rpc_map_add (z_rpc_map_t *self,
                             z_rpc_ctx_t *ctx,
                             z_rpc_callback_t callback,
                             void *ucallback,
                             void *udata)
{
  return(NULL);
}