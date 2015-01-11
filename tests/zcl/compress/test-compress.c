#include <zcl/compress.h>

#include <string.h>
#include <stdio.h>

static void __binary_dump (const uint8_t *buf, uint32_t size) {
  const uint8_t *pbuf = buf;
  while (size --> 0) {
    printf("%02x", *pbuf++);
  }
  printf("\n");
}

static int __test_compress (const z_compress_vtable_t *vtable) {
  z_compressor_t compr;
  uint8_t input[1024];
  uint8_t cbuf[1024];
  uint8_t ubuf[1024];
  uint32_t csize;
  int r;

  memset(input +   0, 10, 128);
  memset(input + 128, 20, 128);
  memset(input + 256, 30, 128);
  memset(input + 384, 40, 128);
  memset(input + 512, 50, 128);
  memset(input + 640, 60, 128);
  memset(input + 768, 70, 128);
  memset(input + 896, 80, 128);
  memset(cbuf, 1, 1024);
  memset(ubuf, 2, 1024);

  compr.level = 10;

  vtable->load(&compr);

  csize = 1024;
  r = vtable->compress(&compr, input, 1024, cbuf, &csize);
  printf("compress r=%d csize=%u - ", r, csize);
  __binary_dump(cbuf, 30);

  r = vtable->decompress(&compr, cbuf, csize, ubuf, 1024);
  printf("decompress r=%d cmp=%d\n", r, memcmp(ubuf, input, 1024));

  vtable->unload(&compr);

  return(0);
}

int main (int argc, char **argv) {
  printf("lz4\n");  __test_compress(&z_compress_lz4);
  printf("zlib\n"); __test_compress(&z_compress_zlib);
  return(0);
}