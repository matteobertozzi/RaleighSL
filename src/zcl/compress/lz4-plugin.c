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

#include <zcl/compress.h>

#include "codec/lz4.h"

static int __lz4_noop (z_compressor_t *self) {
  return(0);
}

static int __lz4_compress (z_compressor_t *self,
                           const uint8_t *src, uint32_t src_len,
                           uint8_t *dst, uint32_t *dst_len)
{
  int r;
  r = LZ4_compress_limitedOutput((const char *)src, (char *)dst, src_len, *dst_len);
  if (r <= 0 || src_len <= r)
    return(-1);
  *dst_len = r;
  return(0);
}

static int __lz4_decompress (z_compressor_t *self,
                             const uint8_t *src, uint32_t src_len,
                             uint8_t *dst, uint32_t dst_len)
{
  int r;
  r = LZ4_decompress_safe((const char *)src, (char *)dst, src_len, dst_len);
  return(r != dst_len);
}

const z_compress_vtable_t z_compress_lz4 = {
  .load       = __lz4_noop,
  .unload     = __lz4_noop,
  .compress   = __lz4_compress,
  .decompress = __lz4_decompress,
};
