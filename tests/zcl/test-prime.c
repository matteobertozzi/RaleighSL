#include <zcl/debug.h>
#include <zcl/time.h>
#include <zcl/math.h>

#include <stdio.h>

#define FIRST_I      (1 << 10)
#define LAST_I       (4 << 20)
#define STEP_I       (8)
#define NITEMS       (LAST_I - FIRST_I)

int main (int argc, char **argv) {
  uint64_t i;

  Z_DEBUG_TOPS(RPRIME, NITEMS, {
    for (i = FIRST_I; i <= LAST_I; i += STEP_I) {
      uint32_t prime = z_prime32(i);
      Z_ASSERT(prime >= i, "invalid value prime=%u i=%u expected >=", prime, i);
    }
  });

  Z_DEBUG_TOPS(LPRIME, NITEMS, {
    for (i = FIRST_I; i <= LAST_I; i += STEP_I) {
      uint32_t prime = z_lprime32(i);
      Z_ASSERT(prime <= i, "invalid value prime=%u i=%u expected <=", prime, i);
    }
  });

  Z_DEBUG_TOPS(CYCPRM, NITEMS, {
    for (i = FIRST_I; i <= LAST_I; i += (STEP_I * 32)) {
      z_cycle32_prime(i);
    }
  });
  return 0;
}
