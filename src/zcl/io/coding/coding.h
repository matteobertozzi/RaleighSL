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

#ifndef _Z_CODING_H_
#define _Z_CODING_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/type.h>
#include <zcl/bits.h>

#define z_encode_zigzag32(n)      (((n) << 1) ^ ((n) >> 31))
#define z_encode_zigzag64(n)      (((n) << 1) ^ ((n) >> 63))
#define z_decode_zigzag32(n)      (((n) >> 1) ^ -((n) & 1))
#define z_decode_zigzag64(n)      (((n) >> 1) ^ -((n) & 1))

#define       z_uint8_size        z_uint32_size
#define       z_uint16_size       z_uint32_size
unsigned int  z_uint32_size       (uint32_t value);
unsigned int  z_uint64_size       (uint64_t value);

#define       z_int8_size         z_int32_size
#define       z_int16_size        z_int32_size
#define       z_int32_size(n)     z_uint32_size(z_encode_zigzag32(n))
#define       z_int64_size(n)     z_uint64_size(z_encode_zigzag64(n))

/* Encode/Decode varint */
unsigned int  z_vint_size         (uint64_t value);
int           z_encode_vint       (uint8_t *buf, uint64_t value);
int           z_decode_vint       (const uint8_t *buf,
                                   unsigned int buflen,
                                   uint64_t *value);

/* Encode/Decode int */
#define       z_encode_int8       z_encode_int
#define       z_encode_int16      z_encode_int
#define       z_encode_int32      z_encode_int
#define       z_encode_int64      z_encode_int

#define       z_encode_uint8      z_encode_uint
#define       z_encode_uint16     z_encode_uint
#define       z_encode_uint32     z_encode_uint
#define       z_encode_uint64     z_encode_uint

#define z_encode_int(buf, len, val)                                    \
  z_encode_uint(buf, len, z_decode_zigzag64(val))

void          z_encode_uint       (uint8_t *buf,
                                   unsigned int length,
                                   uint64_t value);
void          z_decode_uint16     (const uint8_t *buffer,
                                   unsigned int length,
                                   uint16_t *value);
void          z_decode_uint32     (const uint8_t *buffer,
                                   unsigned int length,
                                   uint32_t *value);
void          z_decode_uint64     (const uint8_t *buffer,
                                   unsigned int length,
                                   uint64_t *value);

/* Encode/Decode int groups */
int           z_encode_4int       (uint8_t *buf,
                                   uint32_t a,
                                   uint32_t b,
                                   uint32_t c,
                                   uint32_t d);
int           z_decode_4int       (const uint8_t *buf,
                                   uint32_t *a,
                                   uint32_t *b,
                                   uint32_t *c,
                                   uint32_t *d);
int           z_encode_3int       (uint8_t *buf,
                                   uint64_t a,
                                   uint64_t b,
                                   uint32_t c);
int           z_decode_3int       (const uint8_t *buf,
                                   uint64_t *a,
                                   uint64_t *b,
                                   uint32_t *c);

/* Encode/Decode field */
unsigned int  z_encoded_field_length (uint16_t field_id, uint64_t length);

int           z_encode_field      (uint8_t *buf,
                                   uint16_t field_id,
                                   uint64_t length);
int           z_decode_field      (const uint8_t *buf,
                                   unsigned int buflen,
                                   uint16_t *field_id,
                                   uint64_t *length);

__Z_END_DECLS__

#endif /* !_Z_CODING_H_ */