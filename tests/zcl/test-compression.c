
#include <zcl/compression.h>
#include <zcl/global.h>
#include <zcl/string.h>
#include <zcl/debug.h>
#include <zcl/test.h>

static int __test_compress (z_test_t *test) {
  z_decompressor_t decompressor;
  z_compressor_t compressor;
  unsigned char block[8192];
  unsigned int blksize;
  char buffer[512];
  int nitems;
  int count;
  int i;

  z_compressor_init(&compressor,
                    buffer, sizeof(buffer),
                    block, sizeof(block),
                    z_lz4_compress);
  for (i = 0; i < 1000000; ++i) {
    char str[64];
    int n = snprintf(str, sizeof(str), "Data-%010d:", i);
    if (z_compressor_add(&compressor, str, n)) {
      fprintf(stderr, "break %s\n", str);
      break;
    }
  }
  z_compressor_flush(&compressor);
  nitems = i;
  blksize = z_compressor_flushed_size(&compressor);
  
  count = 0;
  z_decompressor_open(&decompressor, block, blksize, z_lz4_decompress);
  while (1) {
    char buffer[512];
    char *pbuf = buffer;

    int bufsize = z_decompressor_read(&decompressor, buffer, sizeof(buffer));
    if (bufsize <= 0)
      break;

    Z_ASSERT(bufsize <= sizeof(buffer), "Unexpected uncompressed size %d", bufsize);
    while (bufsize > 0) {
      char str[64];
      int n = snprintf(str, sizeof(str), "Data-%010d:", count);
      Z_ASSERT(z_memeq(pbuf, str, n), "Unexpected value %s != %s", str, pbuf);
      pbuf += n;
      bufsize -= n;
      count++;
    }
  }
  z_decompressor_close(&decompressor);
  return((1 + count) != nitems);
}

static int __test_int (z_test_t *test) {
  z_decompressor_t decompressor;
  z_compressor_t compressor;
  unsigned char block[8192];
  unsigned int i, count;
  unsigned int blksize;
  char buffer[512];
  int nitems;

  z_compressor_init(&compressor,
                    buffer, sizeof(buffer),
                    block, sizeof(block),
                    z_plain_compress);
  for (i = 0; i < 1000000; ++i) {
    if (z_compressor_add_int(&compressor, 5)) {
      fprintf(stderr, "break %u\n", i);
      break;
    }
  }
  z_compressor_flush(&compressor);
  nitems = i;
  blksize = z_compressor_flushed_size(&compressor);
  
  count = 0;
  z_decompressor_open(&decompressor, block, blksize, z_plain_decompress);
  while (1) {
    uint8_t buffer[512];
    uint8_t *pbuf = buffer;

    int bufsize = z_decompressor_read(&decompressor, buffer, sizeof(buffer));
    if (bufsize <= 0)
      break;

    Z_ASSERT(bufsize <= sizeof(buffer), "Unexpected uncompressed size %d", bufsize);
    while (bufsize > 0) {
      uint64_t value = 0;
      int n = z_decompressor_get_int(&decompressor, pbuf, bufsize, &value);
      Z_ASSERT(value == 5, "Unexpected value %lu count %u", value, count);
      pbuf += n;
      bufsize -= n;
      count++;
    }
  }
  z_decompressor_close(&decompressor);
  return((1 + count) != nitems);
}

static z_test_t __test_compression = {
  .setup      = NULL,
  .tear_down  = NULL,
  .funcs    = {
    __test_compress,
    __test_int,
    NULL,
  },
};

int main (int argc, char **argv) {
  z_allocator_t allocator;
  int res;

  /* Initialize allocator */
  if (z_system_allocator_open(&allocator))
    return(1);

  /* Initialize global context */
  if (z_global_context_open(&allocator, NULL)) {
    z_allocator_close(&allocator);
    return(1);
  }

  if ((res = z_test_run(&__test_compression, NULL)))
    printf(" [ !! ] Compression %d\n", res);
  else
    printf(" [ ok ] Compression\n");

  z_global_context_close();
  z_allocator_close(&allocator);
  return(res);
}