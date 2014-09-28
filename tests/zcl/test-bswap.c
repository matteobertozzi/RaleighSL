#include <zcl/endian.h>
#include <zcl/debug.h>
#include <zcl/time.h>

#include <stdio.h>

#define FIRST_I      (0)
#define LAST_I       (256 << 20)
#define STEP_I       (1)
#define NITEMS       (LAST_I - FIRST_I)

int main (int argc, char **argv) {
  uint32_t i;

  Z_DEBUG_TOPS(BSWAP32, NITEMS, {
    for (i = FIRST_I; i <= LAST_I; i += STEP_I) {
      uint32_t x = z_bswap32(i);
      x = 0;
    }
  });

  Z_DEBUG_TOPS(BSWAP64, NITEMS, {
    for (i = FIRST_I; i <= LAST_I; i += STEP_I) {
      uint64_t x = ((uint64_t)i << 32) | i;
      x = z_bswap64(x);
    }
  });

  return 0;
}
