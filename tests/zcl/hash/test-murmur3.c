#include <zcl/time.h>
#include <zcl/murmur3.h>

#define M(x)        (x) * 1000000ul
#define NITEMS      M(10)

int main (int argc, char **argv) {
  uint64_t digest[2];
  uint64_t i, seed;
  uint64_t st, et;
  double sec;

  seed = 0;
  st = z_time_micros();
  for (i = 0; i < NITEMS; ++i) {
    z_hash32_murmur3(i, &seed, 8);
  }
  et = (z_time_micros() - st);
  sec = et / 1000000.0f;
  printf("[MURMUR3-32] %llu %.3fM/sec\n", et, (NITEMS / sec) / 1000.0 / 1000.0);

  seed = 0;
  st = z_time_micros();
  for (i = 0; i < NITEMS; ++i) {
    z_hash64_murmur3(i, &seed, 8);
  }
  et = (z_time_micros() - st);
  sec = et / 1000000.0f;
  printf("[MURMUR3-64] %llu %.3fM/sec\n", et, (NITEMS / sec) / 1000.0 / 1000.0);

  seed = 0;
  st = z_time_micros();
  for (i = 0; i < NITEMS; ++i) {
    z_hash128_murmur3(&seed, 8, digest);
  }
  et = (z_time_micros() - st);
  sec = et / 1000000.0f;
  printf("[MURMUR3-128] %llu %.3fM/sec\n", et, (NITEMS / sec) / 1000.0 / 1000.0);

  st = 1;
  uint32_t h32 = z_hash32_murmur3(0, "The quick brown fox jumps over the lazy dog", 43);
  printf("%x\n", h32);
  char buffer[] = {1};
  h32 = z_hash32_murmur3(0, buffer, 1);
  printf("H32 = %x\n", h32);
  digest[0] = 0; digest[1] = 0;
  z_hash128_murmur3("The quick brown fox jumps over the lazy dog", 43, digest);
  printf("%llx %llx\n", digest[0], digest[1]);
  return 0;
}

