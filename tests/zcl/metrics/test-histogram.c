#include <zcl/histogram.h>
#include <zcl/humans.h>
#include <zcl/debug.h>
#include <zcl/rand.h>

#define Z_TEST_ASSERT_EQ(expected, value)                     \
  if ((expected) != (value)) {                                \
    fprintf(stderr, "FAILED %s != %s\n", #expected, #value);  \
    exit(1);                                                  \
  }

static void __verify (z_histogram_t *histo, int nbuckets) {
  int i;

  z_histogram_clear(histo, nbuckets);
  for (i = 0; i < (nbuckets + 4); ++i) {
    z_histogram_add(histo, i);
  }

  for (i = 0; i < (nbuckets - 1); ++i) {
    Z_ASSERT(1 == histo->events[i], "%d: 1 != %u", i, histo->events[i]);
  }
  Z_ASSERT(35 == histo->max, "max %u != 35", histo->max);
  Z_ASSERT(5 == histo->events[i], "%d: 5 != %u", i, histo->events[i]);
}

int main (int argc, char **argv) {
  z_histogram_t histo;
  uint64_t bounds[32];
  uint64_t events[32];
  int i;

  for (i = 0; i < 31; ++i) {
    bounds[i] = i;
  }
  bounds[31] = 0xffffffffffffffffll;

  z_histogram_init(&histo, bounds, events, 32);
  __verify(&histo, 32);

  if (0) {
    uint64_t seed = 1;
    for (i = 0; i < 0x1fffffff; ++i) {
      uint32_t v = z_rand32(&seed) & 31;
      z_histogram_add(&histo, v);
    }
  }
  z_histogram_dump(&histo, stdout, z_human_dsize);

  return(0);
}