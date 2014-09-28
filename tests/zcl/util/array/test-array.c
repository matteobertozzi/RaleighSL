#include <zcl/macros.h>
#include <zcl/endian.h>
#include <zcl/array.h>
#include <zcl/time.h>
#include <zcl/rand.h>

#include <string.h>

#define FANOUT              16
#define SLOT_BLOCK          4096
#define INITIAL_CAPACITY    32
#define NITEMS              (1 << 21)

int main (int argc, char **argv) {
  z_array_t array;
  uint64_t st, et;
  uint64_t seed;
  double sec;
  uint32_t i;

  z_array_open(&array, 4, FANOUT, SLOT_BLOCK, INITIAL_CAPACITY);
  Z_DEBUG("lbase=%u slot_items=%u node_items=%u iper_block=%u",
          array.lbase, array.slot_items, array.node_items, array.iper_block);

  seed = NITEMS;
  st = z_time_micros();
  for (i = 0; i < NITEMS; ++i) {
    uint32_t key = z_bswap32(z_rand32(&seed));
    z_array_resize(&array, i + 1);
    z_array_set_ptr(&array, i, &key);
  }
  et = (z_time_micros() - st);
  sec = et / 1000000.0f;
  Z_DEBUG("[INSERT] %8llu %7.3fM/sec (nodes %u)",
          et, (NITEMS / sec) / 1000.0 / 1000.0, array.node_count);

  seed = NITEMS;
  st = z_time_micros();
  for (i = 0; i < NITEMS; ++i) {
    uint32_t key = z_bswap32(z_rand32(&seed));
    if (z_array_get_u32(&array, i) != key)
      Z_DEBUG(" - NOT FOUND %u", key);
  }
  et = (z_time_micros() - st);
  sec = et / 1000000.0f;
  Z_DEBUG("[LOOKUP] %8llu %7.3fM/sec", et, (NITEMS / sec) / 1000.0 / 1000.0);

  z_array_close(&array);

  z_array_open(&array, 4, FANOUT, SLOT_BLOCK, INITIAL_CAPACITY);

  st = z_time_micros();
  for (i = 0; i < NITEMS; ++i) {
    uint32_t key = z_bswap32(i);
    z_array_resize(&array, i + 1);
    z_array_set_ptr(&array, i, &key);
  }
  et = (z_time_micros() - st);
  sec = et / 1000000.0f;
  Z_DEBUG("[APPEND] %8llu %7.3fM/sec (nodes %u)",
          et, (NITEMS / sec) / 1000.0 / 1000.0, array.node_count);

  st = z_time_micros();
  for (i = 0; i < NITEMS; ++i) {
    uint32_t key = z_bswap32(i);
    if (z_array_get_u32(&array, i) != key)
      Z_DEBUG(" - NOT FOUND %u", key);
  }
  et = (z_time_micros() - st);
  sec = et / 1000000.0f;
  Z_DEBUG("[LOOKUP] %8llu %7.3fM/sec", et, (NITEMS / sec) / 1000.0 / 1000.0);

  z_array_close(&array);
  return 0;
}
