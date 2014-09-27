#include <zcl/macros.h>
#include <zcl/endian.h>
#include <zcl/time.h>
#include <zcl/rand.h>
#include <zcl/avl.h>

#include <string.h>

#define STRIDE    4
#define BLOCK     (64 << 10) * 8
#define NITEMS    (((BLOCK - 16) / (5 + 4)))

int main (int argc, char **argv) {
  uint8_t block[BLOCK];
  uint64_t st, et;
  uint64_t seed;
  double sec;
  uint32_t i;

  z_avl16_init(block, BLOCK, STRIDE);

  seed = NITEMS;
  st = z_time_micros();
  for (i = 0; i < NITEMS; ++i) {
    uint32_t key = z_bswap32(z_rand32(&seed));
    uint8_t *pkey = z_avl16_insert(block, memcmp, &key, 4);
    memcpy(pkey, &key, 4);
  }
  et = (z_time_micros() - st);
  sec = et / 1000000.0f;
  printf("[INSERT] %8llu %.3fM/sec\n", et, (NITEMS / sec) / 1000.0 / 1000.0);

  seed = NITEMS;
  st = z_time_micros();
  for (i = 0; i < NITEMS; ++i) {
    uint32_t key = z_bswap32(z_rand32(&seed));
    if (z_avl16_lookup(block, memcmp, &key, 4) == NULL)
      printf(" - NOT FOUND %u\n", key);
  }
  et = (z_time_micros() - st);
  sec = et / 1000000.0f;
  printf("[LOOKUP] %8llu %.3fM/sec\n", et, (NITEMS / sec) / 1000.0 / 1000.0);

  z_avl16_init(block, BLOCK, STRIDE);
  st = z_time_micros();
  for (i = 0; i < NITEMS; ++i) {
    uint32_t key = z_bswap32(i);
    uint8_t *pkey = z_avl16_append(block, &key, 4);
    memcpy(pkey, &key, 4);
  }
  et = (z_time_micros() - st);
  sec = et / 1000000.0f;
  printf("[APPEND] %8llu %.3fM/sec\n", et, (NITEMS / sec) / 1000.0 / 1000.0);

  st = z_time_micros();
  for (i = 0; i < NITEMS; ++i) {
    uint32_t key = z_bswap32(i);
    if (z_avl16_lookup(block, memcmp, &key, 4) == NULL)
      printf(" - NOT FOUND %u\n", key);
  }
  et = (z_time_micros() - st);
  sec = et / 1000000.0f;
  printf("[LOOKUP] %8llu %.3fM/sec\n", et, (NITEMS / sec) / 1000.0 / 1000.0);

  st = z_time_micros();
  for (i = 0; i < NITEMS; ++i) {
    uint32_t key = z_bswap32(i);
    z_avl16_remove(block, memcmp, &key, 4);
  }
  et = (z_time_micros() - st);
  sec = et / 1000000.0f;
  printf("[REMOVE] %8llu %.3fM/sec\n", et, (NITEMS / sec) / 1000.0 / 1000.0);

  z_avl16_dump(block);
  return 0;
}

