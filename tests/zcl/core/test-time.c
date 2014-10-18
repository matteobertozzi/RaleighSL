#include <zcl/debug.h>
#include <zcl/time.h>

#define NITEMS       (1 << 28)

int main (int argc, char **argv) {

  uint64_t i;
#if 0
  Z_DEBUG_TOPS(TIME_SEC, NITEMS, {
    for (i = 0; i < NITEMS; ++i) {
      z_time_secs();
    }
  });

  Z_DEBUG_TOPS(TIME_MILLIS, NITEMS, {
    for (i = 0; i < NITEMS; ++i) {
      z_time_millis();
    }
  });
#endif

  Z_DEBUG_TOPS(TIME_MICROS, NITEMS, {
    for (i = 0; i < NITEMS; ++i) {
      z_time_micros();
    }
  });

  Z_DEBUG_TOPS(TIME_NANOS, NITEMS, {
    for (i = 0; i < NITEMS; ++i) {
      z_time_nanos();
    }
  });
  return 0;
}
