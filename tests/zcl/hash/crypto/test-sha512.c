#include <zcl/time.h>
#include <zcl/sha512.h>

#define M(x)        (x) * 1000000ul
#define NITEMS      M(1)

int main (int argc, char **argv) {
  uint8_t digest[64];
  uint64_t i, seed;

  seed = 0;
  Z_DEBUG_TOPS(SHA512, NITEMS, {
    for (i = 0; i < NITEMS; ++i) {
      z_rand_sha512(&seed, digest);
    }
  });

  seed = 0;
  z_rand_sha512(&seed, digest);
  printf("{");
  for (i = 0; i < 64; ++i)
    printf("%02x", digest[i]);
  printf("}\n");
  printf("{1b7409ccf0d5a34d3a77eaabfa9fe27427655be9297127ee9522aa1bf4046d4f94");
  printf("5983678169cb1a7348edcac47ef0d9e2c924130e5bcc5f0d94937852c42f1b} EXPECTED\n");
  return 0;
}

