#include <zcl/murmur3.h>
#include <zcl/debug.h>
#include <zcl/time.h>
#include <zcl/rand.h>

#include <string.h>

#define M(x)          (x) * 1000000ul
#define NITEMS        M(10)
#define NITEMS_DIST   10000

static void test_perf (void) {
  Z_DEBUG_TOPS(MURMUR3_32, NITEMS, {
    uint32_t i;
    for (i = 0; i < NITEMS; ++i) {
      z_hash32_murmur3(0, &i, 8);
    }
  });

  Z_DEBUG_TOPS(MURMUR3_64, NITEMS, {
    uint64_t i;
    for (i = 0; i < NITEMS; ++i) {
      z_hash64_murmur3(0, &i, 8);
    }
  });

  Z_DEBUG_TOPS(MURMUR3_128, NITEMS, {
    uint64_t digest[2];
    uint64_t i;
    for (i = 0; i < NITEMS; ++i) {
      digest[0] = 0;
      digest[1] = 0;
      z_hash128_murmur3(&i, 8, digest);
    }
  });
}

static void test_validate (void) {
  uint64_t digest[2];
  uint32_t h32 = z_hash32_murmur3(0, "The quick brown fox jumps over the lazy dog", 43);
  printf("%x\n", h32);
  char buffer[] = {1};
  h32 = z_hash32_murmur3(0, buffer, 1);
  printf("H32 = %x\n", h32);
  digest[0] = 0; digest[1] = 0;
  z_hash128_murmur3("The quick brown fox jumps over the lazy dog", 43, digest);
  printf("%"PRIx64" %"PRIx64"\n", digest[0], digest[1]);
  /* TODO */
}

static void test_distribution (void) {
  uint64_t buckets[128];
  int i, b;

  for (b = 2; b < 128; ++b) {
    uint64_t x, seed;

    memset(buckets, 0, sizeof(buckets));
    for (x = 0; x < NITEMS_DIST; ++x)
      buckets[z_hash32_murmur3(0, &x, 8) % b]++;
    Z_DEBUG("murmur3 32bit seq-distribution buckets=%d", b);
    for (i = 0; i < b; ++i) {
      double d = (i == 0) ? 0 :
        (buckets[i] > buckets[i-1]) ? (double)buckets[i-1] / buckets[i]
          : (double)buckets[i] / buckets[i-1];
      Z_DEBUG(" - bucket[%d] = %"PRIu64" diff=%.3f%%", i, buckets[i], d);
    }

    memset(buckets, 0, sizeof(buckets));
    for (x = 0; x < NITEMS_DIST; ++x)
      buckets[z_hash64_murmur3(0, &x, 8) % b]++;
    Z_DEBUG("murmur3 64bit seq-distribution buckets=%d", b);
    for (i = 0; i < b; ++i) {
      double d = (i == 0) ? 0 :
        (buckets[i] > buckets[i-1]) ? (double)buckets[i-1] / buckets[i]
          : (double)buckets[i] / buckets[i-1];
      Z_DEBUG(" - bucket[%d] = %"PRIu64" diff=%.3f%%", i, buckets[i], d);
    }

    memset(buckets, 0, sizeof(buckets));
    seed = NITEMS_DIST;
    for (x = 0; x < NITEMS_DIST; ++x) {
      uint64_t x = z_rand64(&seed);
      buckets[z_hash32_murmur3(0, &x, 8) % b]++;
    }
    Z_DEBUG("murmur3 32bit rand-distribution buckets=%d", b);
    for (i = 0; i < b; ++i) {
      double d = (i == 0) ? 0 :
        (buckets[i] > buckets[i-1]) ? (double)buckets[i-1] / buckets[i]
          : (double)buckets[i] / buckets[i-1];
      Z_DEBUG(" - bucket[%d] = %"PRIu64" diff=%.3f%%", i, buckets[i], d);
    }

    memset(buckets, 0, sizeof(buckets));
    seed = NITEMS_DIST;
    for (x = 0; x < NITEMS_DIST; ++x) {
      uint64_t x = z_rand64(&seed);
      buckets[z_hash64_murmur3(0, &x, 8) % b]++;
    }
    Z_DEBUG("murmur3 64bit rand-distribution buckets=%d", b);
    for (i = 0; i < b; ++i) {
      double d = (i == 0) ? 0 :
        (buckets[i] > buckets[i-1]) ? (double)buckets[i-1] / buckets[i]
          : (double)buckets[i] / buckets[i-1];
      Z_DEBUG(" - bucket[%d] = %"PRIu64" diff=%.3f%%", i, buckets[i], d);
    }
  }
}

int main (int argc, char **argv) {
  test_perf();
  test_validate();
  test_distribution();
  return(0);
}

