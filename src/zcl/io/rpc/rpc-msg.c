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

#include <zcl/global.h>
#include <zcl/coding.h>
#include <zcl/string.h>
#include <zcl/debug.h>
#include <zcl/rpc.h>

/* ============================================================================
 *  PUBLIC RPC Msg methods
 */
int z_v_reader_rpc_parse_head (const z_vtable_reader_t *vtable, void *self,
                               uint64_t *msg_type,
                               uint64_t *req_id,
                               int *is_req)
{
  const uint8_t *data;
  uint8_t len[2];
  size_t n;

  n = vtable->next(self, &data);
  if (Z_UNLIKELY(n == 0))
    return(-1);

  len[0]  = 1 + z_fetch_3bit(data[0], 5);
  len[1]  = 1 + z_fetch_3bit(data[0], 2);
  *is_req = z_fetch_1bit(data[0], 1);
  ++data;

  if (Z_LIKELY(n >= (1 + len[0] + len[1]))) {
    z_decode_uint64(data, len[0], msg_type); data += len[0];
    z_decode_uint64(data, len[1], req_id);
    vtable->backup(self, n - (1 + len[0] + len[1]));
  } else if (n >= (1 + len[0])) {
    z_decode_uint64(data, len[0], msg_type); data += len[0];
    vtable->backup(self, n - (1 + len[0]));
    z_v_reader_decode_uint64(vtable, self, len[1], req_id);
  } else {
    vtable->backup(self, n - 1);
    z_v_reader_decode_uint64(vtable, self, len[0], msg_type);
    z_v_reader_decode_uint64(vtable, self, len[1], req_id);
  }

  return(0);
}

int z_rpc_write_head (unsigned char *buffer,
                      uint64_t msg_type,
                      uint64_t req_id,
                      int is_req)
{
  uint8_t *p = buffer;
  uint8_t len[2];

  len[0] = z_uint64_size(msg_type);
  len[1] = z_uint64_size(req_id);

  *p++ = ((len[0] - 1) << 5) | ((len[1] - 1) << 2) | ((!!is_req) << 1);
  z_encode_uint(p, len[0], msg_type); p += len[0];
  z_encode_uint(p, len[1], req_id);
  return(1 + len[0] + len[1]);
}
