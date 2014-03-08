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

#ifndef _Z_DATA_BYTES_PACKING_H_
#define _Z_DATA_BYTES_PACKING_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

Z_TYPEDEF_STRUCT(z_bytes_packer)
Z_TYPEDEF_STRUCT(z_bytes_unpacker)

struct z_bytes_packer {
  uint32_t nitems;
  uint32_t last_bsize;
  uint32_t last_size;
  uint8_t *last_item;
  uint8_t *head;
};

struct z_bytes_unpacker {
  uint32_t nitems;
  uint32_t last_bsize;
  uint32_t last_size;
  uint8_t  head;
  uint8_t  pad[3];

  uint32_t buf_avail;
  uint8_t *last_item;
  const uint8_t *pbuffer;
};

void     z_bytes_unpacker_open (z_bytes_unpacker_t *self,
                                uint8_t *last_item_buf,
                                const uint8_t *buffer,
                                uint32_t size);
int      z_bytes_unpacker_next (z_bytes_unpacker_t *self);

int      z_bytes_unpacker_ssearch (z_bytes_unpacker_t *self,
                                   const void *key,
                                   uint32_t ksize,
                                   uint32_t *index);
int      z_bytes_unpacker_seek    (z_bytes_unpacker_t *self,
                                   int index);


void     z_bytes_packer_open   (z_bytes_packer_t *self);
uint8_t *z_bytes_packer_close  (z_bytes_packer_t *self,
                                uint8_t *obuf);
uint8_t *z_bytes_packer_add    (z_bytes_packer_t *self,
                                const void *data,
                                uint32_t size,
                                uint8_t *obuf);

__Z_END_DECLS__

#endif /* _Z_DATA_BYTES_PACKING_H_ */
