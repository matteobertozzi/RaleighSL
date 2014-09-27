#include <zcl/time.h>
#include <zcl/md5.h>

#define M(x)        (x) * 1000000ul
#define NITEMS      M(1)

int main (int argc, char **argv) {
  uint8_t digest[16];
  uint64_t i, seed;
  uint64_t st, et;
  double sec;

  seed = 0;
  st = z_time_micros();
  for (i = 0; i < NITEMS; ++i) {
    z_rand_md5(&seed, digest);
  }
  et = (z_time_micros() - st);
  sec = et / 1000000.0f;
  printf("[MD5] %8llu %.3fM/sec\n", et, (NITEMS / sec) / 1000.0 / 1000.0);

  seed = 0;
  z_rand_md5(&seed, digest);
  printf("{");
  for (i = 0; i < 16; ++i)
    printf("%02x", digest[i]);
  printf("}\n");
  printf("{7dea362b3fac8e00956a4952a3d4f474} EXPECTED\n");
  return 0;
}

