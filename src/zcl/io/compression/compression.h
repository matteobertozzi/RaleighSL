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

#ifndef _Z_COMPRESSION_H_
#define _Z_COMPRESSION_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/slice.h>

Z_TYPEDEF_STRUCT(z_decompressor)
Z_TYPEDEF_STRUCT(z_compressor)


typedef unsigned int (*z_compress_t)   (uint8_t *dst, 
                                        unsigned int dst_size,
                                        const uint8_t *src,
                                        unsigned int src_size);
typedef unsigned int (*z_decompress_t) (uint8_t *dst,
                                        unsigned int dst_size,
                                        const uint8_t *src);

struct z_compressor {
  uint8_t *dst_buffer;   
  unsigned int dst_avail;
  unsigned int dst_size;
  
  uint8_t *buffer; 
  unsigned int bufavail;
  unsigned int bufsize;

  z_compress_t codec;
  z_byte_slice_t bprev;
  uint64_t iprev;
};

struct z_decompressor {
  const uint8_t *buffer;
  unsigned int bufsize;
  z_decompress_t codec;
  z_byte_slice_t body;
  z_slice_t prefix;
  z_slice_t suffix;
  uint64_t iprev;
};

unsigned int  z_lz4_compress     (uint8_t *dst, unsigned int dst_size,
                                  const uint8_t *src, unsigned int src_size);
unsigned int  z_lz4_decompress   (uint8_t *dst, unsigned int dst_size, const uint8_t *src);
unsigned int  z_plain_compress   (uint8_t *dst, unsigned int dst_size,
                                  const uint8_t *src, unsigned int src_size);
unsigned int  z_plain_decompress (uint8_t *dst, unsigned int dst_size, const uint8_t *src);

void          z_compressor_init           (z_compressor_t *self,
                                           void *buffer,
                                           unsigned int bufsize,
                                           void *dst_buffer,
                                           unsigned int dst_size,
                                           z_compress_t codec);
int           z_compressor_add            (z_compressor_t *self,
                                           const void *data,
                                           unsigned int size);
int           z_compressor_addv           (z_compressor_t *self,
                                           const struct iovec *iov,
                                           int iovcnt);
int           z_compressor_add_bytes      (z_compressor_t *self,
                                           const z_byte_slice_t *current);
int           z_compressor_add_int        (z_compressor_t *self,
                                           uint64_t current);
int           z_compressor_flush          (z_compressor_t *self);
unsigned int  z_compressor_flushed_size   (const z_compressor_t *self);
unsigned int  z_compressor_estimated_size (const z_compressor_t *self);


void          z_decompressor_open         (z_decompressor_t *self,
                                           const void *buffer,
                                           unsigned int bufsize,
                                           z_decompress_t codec);
void          z_decompressor_close        (z_decompressor_t *self);
int           z_decompressor_read         (z_decompressor_t *self,
                                           void *dst,
                                           unsigned int size);
int           z_decompressor_get_bytes    (z_decompressor_t *self,
                                           const uint8_t *buffer,
                                           unsigned int bufsize,
                                           z_slice_t *current);
int           z_decompressor_get_int      (z_decompressor_t *self,
                                           const uint8_t *buffer,
                                           unsigned int bufsize,
                                           uint64_t *current);

__Z_END_DECLS__

#endif /* _Z_COMPRESSION_H_ */
