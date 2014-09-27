#include <zcl/time.h>
#include <zcl/sha256.h>

#define M(x)        (x) * 1000000ul
#define NITEMS      M(1)

int main (int argc, char **argv) {
  uint8_t digest[32];
  uint64_t i, seed;
  uint64_t st, et;
  double sec;

  seed = 0;
  st = z_time_micros();
  for (i = 0; i < NITEMS; ++i) {
    z_rand_sha256(&seed, digest);
  }
  et = (z_time_micros() - st);
  sec = et / 1000000.0f;
  printf("[SHA256] %llu %.3fM/sec\n", et, (NITEMS / sec) / 1000.0 / 1000.0);

  seed = 0;
  z_rand_sha256(&seed, digest);
  printf("{");
  for (i = 0; i < 32; ++i)
    printf("%02x", digest[i]);
  printf("}\n");
  printf("{af5570f5a1810b7af78caf4bc70a660f0df51e42baf91d4de5b2328de0e83dfc} EXPECTED\n");
  return 0;
}

