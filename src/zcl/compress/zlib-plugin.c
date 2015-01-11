#include "compress.h"

#include <zlib.h>

static int __zlib_load (z_compressor_t *self) {
  if (self->level > 9) {
    self->level = Z_BEST_COMPRESSION;
  } else if (self->level < 0) {
    self->level = Z_DEFAULT_COMPRESSION;
  }
  return(0);
}

static int __zlib_unload (z_compressor_t *self) {
  return(0);
}

static int __zlib_compress (z_compressor_t *self,
                            const uint8_t *src, uint32_t src_len,
                            uint8_t *dst, uint32_t *dst_len)
{
  uLongf destLen;
  int r;

  destLen = *dst_len;
  r = compress2(dst, &destLen, src, src_len, self->level);
  if (r != Z_OK || destLen >= src_len)
    return(-1);
  *dst_len = destLen;
  return(0);
}

static int __zlib_decompress (z_compressor_t *self,
                              const uint8_t *src, uint32_t src_len,
                              uint8_t *dst, uint32_t dst_len)
{
  uLongf destLen;
  int r;

  destLen = dst_len;
  r = uncompress(dst, &destLen, src, src_len);
  return(r != Z_OK || destLen != dst_len);
}

const z_compress_vtable_t z_compress_zlib = {
  .load       = __zlib_load,
  .unload     = __zlib_unload,
  .compress   = __zlib_compress,
  .decompress = __zlib_decompress,
};
