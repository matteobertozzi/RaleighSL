#include <zcl/mt19937.h>

#define N  312
#define M  156
#define UPPER_MASK  0xFFFFFFFF80000000ull
#define LOWER_MASK  0x7FFFFFFFULL

void z_mt19937_64_init (z_mt19937_64_t *self, uint64_t seed) {
  uint64_t *mt = self->mt;
  int i;
  mt[0] = seed != 0 ? seed : 5489ull;
  for (i = 1; i < N; ++i) {
    mt[i] = (6364136223846793005ULL * (mt[i-1] ^ (mt[i-1] >> 62)) + i);
  }
  self->mti = N;
}

static void __mt19937_64_next (z_mt19937_64_t *self) {
  const uint64_t mag01[2] = {0ULL, 0xB5026F5AA96619E9ull};
  uint64_t *mt = self->mt;
  uint64_t x;
  int i;

  for (i=0; i< N - M; ++i) {
    x = (mt[i] & UPPER_MASK) | (mt[i + 1] & LOWER_MASK);
    mt[i] = mt[i + M] ^ (x >> 1) ^ mag01[x & 1];
  }
  for (; i < N - 1; i++) {
    x = (mt[i] & UPPER_MASK) | (mt[i + 1] & LOWER_MASK);
    mt[i] = mt[i + (M - N)] ^ (x >> 1) ^ mag01[x & 1];
  }
  x = (mt[N - 1] & UPPER_MASK) | (mt[0] & LOWER_MASK);
  mt[N - 1] = mt[M - 1] ^ (x >> 1) ^ mag01[x & 1];
  self->mti = 0;
}

uint64_t z_mt19937_64_next (z_mt19937_64_t *self) {
  uint64_t x;

  if (self->mti >= N) {
    __mt19937_64_next(self);
  }

  x = self->mt[self->mti++];
  x ^= (x >> 29) & 0x5555555555555555ULL;
  x ^= (x << 17) & 0x71D67FFFEDA60000ULL;
  x ^= (x << 37) & 0xFFF7EEE000000000ULL;
  x ^= (x >> 43);
  return(x);
}
