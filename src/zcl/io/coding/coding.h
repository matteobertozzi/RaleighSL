/*
 *   Copyright 2011-2013 Matteo Bertozzi
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

#ifndef _Z_CODING_H_
#define _Z_CODING_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/type.h>
#include <zcl/bits.h>

unsigned int  z_uint32_bytes      (uint32_t value);
unsigned int  z_uint64_bytes      (uint64_t value);

/* Encode/Decode int */
void          z_encode_uint       (unsigned char *buf,
                                   unsigned int length,
                                   uint64_t value);
void          z_decode_uint16     (const unsigned char *buffer,
                                   unsigned int length,
                                   uint16_t *value);
void          z_decode_uint32     (const unsigned char *buffer,
                                   unsigned int length,
                                   uint32_t *value);
void          z_decode_uint64     (const unsigned char *buffer,
                                   unsigned int length,
                                   uint64_t *value);

/* Encode/Decode int groups */
int           z_encode_4int       (unsigned char *buf,
                                   uint32_t a,
                                   uint32_t b,
                                   uint32_t c,
                                   uint32_t d);
int           z_decode_4int       (const unsigned char *buf,
                                   uint32_t *a,
                                   uint32_t *b,
                                   uint32_t *c,
                                   uint32_t *d);
int           z_encode_3int       (unsigned char *buf,
                                   uint64_t a,
                                   uint64_t b,
                                   uint32_t c);
int           z_decode_3int       (const unsigned char *buf,
                                   uint64_t *a,
                                   uint64_t *b,
                                   uint32_t *c);

/* Encode/Decode field */
int           z_encode_field      (unsigned char *buf,
                                   uint16_t field_id,
                                   uint64_t length);
int           z_decode_field      (const unsigned char *buf,
                                   unsigned int buflen,
                                   uint16_t *field_id,
                                   uint64_t *length);

__Z_END_DECLS__

#endif /* !_Z_CODING_H_ */