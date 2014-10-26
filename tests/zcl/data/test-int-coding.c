#include <zcl/int-coding.h>
#include <zcl/debug.h>
#include <zcl/time.h>

#include <stdio.h>

static void test_perf_usize (void) {
  uint64_t i;

  Z_DEBUG_TOPS(U16_SIZE, 1000 * (1 << 16), {
    uint32_t j;
    for (j = 0; j < 10000; ++j) {
      for (i = 0; i < (1 << 16); ++i) {
        uint8_t x = z_uint16_size(i & 0xffff);
        x = 0;
      }
    }
  });

  Z_DEBUG_TOPS(U32_SIZE, (1ull << 32) / 16, {
    for (i = 0; i < (1ull << 32); i += 16) {
      uint8_t x = z_uint32_size(i & 0xffffffff);
      x = 0;
    }
  });
}

static void test_perf_int (void) {
  uint64_t i;

  Z_DEBUG_TOPS(U32_ENC, (1ull << 32) / 16, {
    for (i = 0; i < (1ull << 32); i += 16) {
      uint8_t buffer[16];
      z_uint_encode(buffer, 4, i);
    }
  });

  Z_DEBUG_TOPS(U64_ENC, 63000000, {
    uint32_t j;
    for (j = 0; j < 1000000; ++j) {
      for (i = 1; i < (1ull << 63); i = (i << 1)) {
        uint8_t buffer[16];
        z_uint_encode(buffer, 8, i);
      }
    }
  });
}

static void test_perf_vint (void) {
  uint64_t i;

  Z_DEBUG_TOPS(VU32_ENC, (1ull << 32) / 16, {
    for (i = 0; i < (1ull << 32); i += 16) {
      uint8_t buffer[16];
      z_vint32_encode(buffer, i);
    }
  });

  Z_DEBUG_TOPS(VU64_ENC, 63000000, {
    uint32_t j;
    for (j = 0; j < 1000000; ++j) {
      for (i = 1; i < (1ull << 63); i = (i << 1)) {
        uint8_t buffer[16];
        z_vint64_encode(buffer, i);
      }
    }
  });
}

int main (int argc, char **argv) {
  test_perf_usize();
  test_perf_int();
  test_perf_vint();
  return 0;
}
