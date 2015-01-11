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
