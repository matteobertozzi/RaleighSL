#include <zcl/histogram.h>
#include <zcl/math.h>

#define Z_TEST_ASSERT_EQ(expected, value)                     \
  if ((expected) != (value)) {                                \
    fprintf(stderr, "FAILED %s != %s\n", #expected, #value);  \
    exit(1);                                                  \
  }

static void __verify (z_histogram_t *histo) {
  int i;

  z_histogram_clear(histo);
  for (i = 0; i < 33; ++i) {
    z_histogram_add(histo, i);
  }

  for (i = 0; i < 31; ++i) {
    Z_TEST_ASSERT_EQ(1, histo->events[i]);
  }
  Z_TEST_ASSERT_EQ(2, histo->events[31]);
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
  __verify(&histo);

  if (0) {
    unsigned int seed = 1;
    for (i = 0; i < 0x1fffffff; ++i) {
      unsigned int v = z_rand(&seed) & 31;
      z_histogram_add(&histo, v);
    }
  }
  //z_histogram_dump(&histo, stdout, z_human_size);

  return(0);
}