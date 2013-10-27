#include <zcl/compression.h>
#include <zcl/string.h>

/* ============================================================================
 *  LZ4 Codec
 */
extern int LZ4_compress_limitedOutput (const char* source, char* dest, int inputSize, int maxOutputSize);
extern int LZ4_decompress_fast (const char* source, char* dest, int outputSize);

unsigned int z_lz4_compress (uint8_t *dst, unsigned int dst_size,
                             const uint8_t *src, unsigned int src_size)
{
  return(LZ4_compress_limitedOutput((const char *)src, (char *)dst, src_size, dst_size));
}

unsigned int z_lz4_decompress (uint8_t *dst, unsigned int dst_size, const uint8_t *src) {
  return(LZ4_decompress_fast((const char *)src, (char *)dst, dst_size));
}

/* ============================================================================
 *  NONE Codec
 */
unsigned int z_plain_compress (uint8_t *dst, unsigned int dst_size,
                               const uint8_t *src, unsigned int src_size)
{
  if (src_size > dst_size)
    return(0);
  z_memcpy(dst, src, src_size);
  return(src_size);
}

unsigned int z_plain_decompress (uint8_t *dst, unsigned int dst_size, const uint8_t *src) {
  z_memcpy(dst, src, dst_size);
  return(dst_size);
}
