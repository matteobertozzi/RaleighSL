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

#ifndef _Z_DATA_INT_PACKING_H_
#define _Z_DATA_INT_PACKING_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

Z_TYPEDEF_STRUCT(z_uint_packer)
Z_TYPEDEF_STRUCT(z_uint_unpacker)

struct z_uint_packer {
  uint64_t last_value;
  uint64_t last_delta;

  uint8_t count;
  uint8_t ndeltas;
  uint8_t singles;

  uint8_t width;
  uint8_t min_width;
  uint8_t max_width;

  uint8_t buf_used;
  uint8_t buffer[41];
};

struct z_uint_unpacker {
  uint64_t value;
  uint64_t delta;

  uint8_t count;
  uint8_t windex;
  uint8_t whead[3];
  uint8_t pad[3];

  uint8_t fixw;
  uint8_t width;
  uint8_t singles;
  uint8_t ndeltas;

  uint32_t nitems;
  uint32_t pad1;

  uint32_t buf_avail;
  const uint8_t *pbuffer;
};

void     z_uint_unpacker_open    (z_uint_unpacker_t *self,
                                  const uint8_t *buffer,
                                  uint32_t size);
int      z_uint_unpacker_next    (z_uint_unpacker_t *self);
int      z_uint_unpacker_ssearch (z_uint_unpacker_t *self,
                                  uint64_t key,
                                  uint32_t *index);
int      z_uint_unpacker_seek    (z_uint_unpacker_t *self,
                                  int index);


void     z_uint_packer_open    (z_uint_packer_t *self,
                                uint64_t value);
uint8_t *z_uint_packer_close   (z_uint_packer_t *self,
                                uint8_t *obuf);
uint8_t *z_uint_packer_add     (z_uint_packer_t *self,
                                uint64_t value,
                                uint8_t *obuf);

__Z_END_DECLS__

#endif /* !_Z_DATA_INT_PACKING_H_ */
