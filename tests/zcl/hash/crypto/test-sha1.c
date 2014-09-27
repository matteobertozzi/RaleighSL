#include <zcl/time.h>
#include <zcl/sha1.h>

#define M(x)        (x) * 1000000ul
#define NITEMS      M(1)

int main (int argc, char **argv) {
  uint8_t digest[20];
  uint64_t i, seed;
  uint64_t st, et;
  double sec;

  seed = 0;
  st = z_time_micros();
  for (i = 0; i < NITEMS; ++i) {
    z_rand_sha1(&seed, digest);
  }
  et = (z_time_micros() - st);
  sec = et / 1000000.0f;
  printf("[SHA1] %llu %.3fM/sec\n", et, (NITEMS / sec) / 1000.0 / 1000.0);

  seed = 0;
  z_rand_sha1(&seed, digest);
  printf("{");
  for (i = 0; i < 20; ++i)
    printf("%02x", digest[i]);
  printf("}\n");
  printf("{05fe405753166f125559e7c9ac558654f107c7e9} EXPECTED\n");
  return 0;
}

