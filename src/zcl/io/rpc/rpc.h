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

#ifndef _Z_RPC_H_
#define _Z_RPC_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/dbuffer.h>
#include <zcl/macros.h>

Z_TYPEDEF_STRUCT(z_rpc_field_iter)
Z_TYPEDEF_STRUCT(z_rpc_ctx)

struct z_rpc_ctx {

};

struct z_rpc_field_iter {
  const uint8_t *pbuffer;
  int remaining;
  int avail;
};

int  z_rpc_write_uint8     (uint8_t *block, uint16_t field_id, uint8_t  value);
int  z_rpc_write_uint16    (uint8_t *block, uint16_t field_id, uint16_t value);
int  z_rpc_write_uint32    (uint8_t *block, uint16_t field_id, uint32_t value);
int  z_rpc_write_uint64    (uint8_t *block, uint16_t field_id, uint64_t value);

int  z_rpc_uint8_pack1     (uint8_t *buffer, uint8_t v);
int  z_rpc_uint8_pack2     (uint8_t *buffer, const uint8_t v[2]);
int  z_rpc_uint8_pack3     (uint8_t *buffer, const uint8_t v[3]);
int  z_rpc_uint8_pack4     (uint8_t *buffer, const uint8_t v[4]);

int  z_rpc_uint16_pack1    (uint8_t *buffer, uint16_t v);
int  z_rpc_uint16_pack2    (uint8_t *buffer, const uint16_t v[2]);
int  z_rpc_uint16_pack3    (uint8_t *buffer, const uint16_t v[3]);
int  z_rpc_uint16_pack4    (uint8_t *buffer, const uint16_t v[4]);

int  z_rpc_uint32_pack1    (uint8_t *buffer, uint32_t v);
int  z_rpc_uint32_pack2    (uint8_t *buffer, const uint32_t v[2]);
int  z_rpc_uint32_pack3    (uint8_t *buffer, const uint32_t v[3]);
int  z_rpc_uint32_pack4    (uint8_t *buffer, const uint32_t v[4]);

int  z_rpc_uint64_pack1    (uint8_t *buffer, uint64_t v);
int  z_rpc_uint64_pack2    (uint8_t *buffer, const uint64_t v[2]);
int  z_rpc_uint64_pack3    (uint8_t *buffer, const uint64_t v[3]);
int  z_rpc_uint64_pack4    (uint8_t *buffer, const uint64_t v[4]);

void z_rpc_uint8_unpack4   (const uint8_t *buf, uint8_t v[4]);
int  z_rpc_uint8_unpack4c  (const uint8_t *buf, uint32_t length, uint8_t v[4]);

void z_rpc_uint16_unpack4  (const uint8_t *buf, uint16_t v[4]);
int  z_rpc_uint16_unpack4c (const uint8_t *buf, uint32_t length, uint16_t v[4]);

void z_rpc_uint32_unpack4  (const uint8_t *buf, uint32_t v[4]);
int  z_rpc_uint32_unpack4c (const uint8_t *buf, uint32_t length, uint32_t v[4]);

void z_rpc_uint64_unpack4  (const uint8_t *buf, uint64_t v[4]);
int  z_rpc_uint64_unpack4c (const uint8_t *buf, uint32_t length, uint64_t v[4]);

#define z_rpc_iter_field_init(self, block, size)                \
  do {                                                          \
    (self)->pbuffer = (block) + 1;                              \
    (self)->remaining = *(block);                               \
    (self)->avail = size;                                       \
  } while (0)

int  z_rpc_iter_field_uint8  (z_rpc_field_iter_t *self,
                              uint16_t field_id,
                              uint8_t *value);
int  z_rpc_iter_field_uint16 (z_rpc_field_iter_t *self,
                              uint16_t field_id,
                              uint16_t *value);
int  z_rpc_iter_field_uint32 (z_rpc_field_iter_t *self,
                              uint16_t field_id,
                              uint32_t *value);
int  z_rpc_iter_field_uint64 (z_rpc_field_iter_t *self,
                              uint16_t field_id,
                              uint64_t *value);

#define __z_rpc_read_uint(block, type, field_id, value) ({    \
  z_rpc_field_iter_t iter;                                    \
  z_rpc_iter_field_init(&iter, block, 0xffff);                \
  z_rpc_iter_field_uint ## type (&iter, field_id, value);     \
})

#define z_rpc_read_uint8(block, field_id, value)    \
  __z_rpc_read_uint(block, 8, field_id, value)

#define z_rpc_read_uint16(block, field_id, value)    \
  __z_rpc_read_uint(block, 16, field_id, value)

#define z_rpc_read_uint32(block, field_id, value)    \
  __z_rpc_read_uint(block, 32, field_id, value)

#define z_rpc_read_uint64(block, field_id, value)    \
  __z_rpc_read_uint(block, 64, field_id, value)

uint8_t * z_rpc_write_mark8       (z_dbuf_writer_t *writer,
                                   uint16_t field_id);
uint16_t *z_rpc_write_mark16      (z_dbuf_writer_t *writer,
                                   uint16_t field_id);

uint16_t  z_rpc_write_field_count (z_dbuf_writer_t *writer,
                                   uint8_t field_count);
uint16_t  z_rpc_write_u8_list     (z_dbuf_writer_t *writer,
                                   const uint8_t *v, int count);
uint16_t  z_rpc_write_u16_list    (z_dbuf_writer_t *writer,
                                   const uint16_t *v, int count);
uint16_t  z_rpc_write_u32_list    (z_dbuf_writer_t *writer,
                                   const uint32_t *v, int count);
uint16_t  z_rpc_write_u64_list    (z_dbuf_writer_t *writer,
                                   const uint64_t *v, int count);

__Z_END_DECLS__

#endif /* _Z_RPC_H_ */
