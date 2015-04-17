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

#ifndef _Z_COMPRESS_H_
#define _Z_COMPRESS_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

typedef struct z_compress_vtable z_compress_vtable_t;
typedef struct z_compressor z_compressor_t;

typedef int (*z_compress_t)   (z_compressor_t *self,
                               const uint8_t *src, uint32_t src_len,
                               uint8_t *dst, uint32_t *dst_len);
typedef int (*z_decompress_t) (z_compressor_t *self,
                               const uint8_t *src, uint32_t src_len,
                               uint8_t *dst, uint32_t dst_len);

struct z_compressor {
  int level;
};

struct z_compress_vtable {
  int (*load)   (z_compressor_t *self);
  int (*unload) (z_compressor_t *self);

  z_compress_t   compress;
  z_decompress_t decompress;
};

extern const z_compress_vtable_t z_compress_zlib;
extern const z_compress_vtable_t z_compress_lz4;

#endif /* !_Z_COMPRESS_H_ */
