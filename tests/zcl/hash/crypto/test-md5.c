#include <zcl/time.h>
#include <zcl/md5.h>

#define M(x)        (x) * 1000000ul
#define NITEMS      M(1)

int main (int argc, char **argv) {
  uint8_t digest[16];
  uint64_t i, seed;

  seed = 0;
  Z_DEBUG_TOPS(MD5, NITEMS, {
    for (i = 0; i < NITEMS; ++i) {
      z_rand_md5(&seed, digest);
    }
  });

  seed = 0;
  z_rand_md5(&seed, digest);
  printf("{");
  for (i = 0; i < 16; ++i)
    printf("%02x", digest[i]);
  printf("}\n");
  printf("{7dea362b3fac8e00956a4952a3d4f474} EXPECTED\n");
  return 0;
}

