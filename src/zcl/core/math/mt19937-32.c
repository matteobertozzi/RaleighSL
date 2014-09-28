#include <zcl/mt19937.h>

#define N 624
#define M 397
#define UPPER_MASK 0x80000000UL
#define LOWER_MASK 0x7fffffffUL

void z_mt19937_32_init (z_mt19937_32_t *self, uint32_t seed) {
  uint32_t *mt = self->mt;
  int i;
  mt[0] = seed;
  for (i = 1; i < N; ++i) {
    mt[i] = (1812433253u * (mt[i - 1] ^ (mt[i - 1] >> 30)) + i);
  }
  self->mti = N;
}

static void __mt19937_32_next (z_mt19937_32_t *self) {
  static uint32_t mag01[2] = {0x0, 0x9908b0df};
  uint32_t *mt = self->mt;
  uint32_t x;
  int i;

  for (i = 0; i < N-M; ++i) {
    x = (mt[i] & UPPER_MASK) | (mt[i + 1] & LOWER_MASK);
    mt[i] = mt[i + M] ^ (x >> 1) ^ mag01[x & 1];
  }
  for (; i < N - 1; ++i) {
    x = (mt[i]&UPPER_MASK)|(mt[i+1]&LOWER_MASK);
    mt[i] = mt[i + (M - N)] ^ (x >> 1) ^ mag01[x & 1];
  }
  x = (mt[N - 1] & UPPER_MASK) | (mt[0] & LOWER_MASK);
  mt[N - 1] = mt[M - 1] ^ (x >> 1) ^ mag01[x & 1];

  self->mti = 0;
}

uint32_t z_mt19937_32_next (z_mt19937_32_t *self) {
  uint32_t x;

  if (self->mti >= N) {
    __mt19937_32_next(self);
  }

  x = self->mt[self->mti++];
  x ^= (x >> 11);
  x ^= (x << 7) & 0x9d2c5680UL;
  x ^= (x << 15) & 0xefc60000UL;
  x ^= (x >> 18);
  return(x);
}
