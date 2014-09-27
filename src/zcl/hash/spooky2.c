// http://burtleburtle.net/bob/c/SpookyV2.cpp

// Spooky Hash
// A 128-bit noncryptographic hash, for checksums and table lookup
// By Bob Jenkins.  Public domain.
//   Oct 31 2010: published framework, disclaimer ShortHash isn't right
//   Nov 7 2010: disabled ShortHash
//   Oct 31 2011: replace End, ShortMix, ShortEnd, enable ShortHash again
//   April 10 2012: buffer overflow on platforms without unaligned reads
//   July 12 2012: was passing out variables in final to in/out in short
//   July 30 2012: I reintroduced the buffer overflow
//   August 5 2012: SpookyV2: d = should be d += in short hash, and remove extra mix from long hash

#include <string.h>

#include <zcl/spooky2.h>
#include <zcl/bits.h>

#define ALLOW_UNALIGNED_READS 1

#define __SC_BLOCKSIZE        (12 * 8)
#define __SC_BUFSIZE          (2 * __SC_BLOCKSIZE)
#define __SC_CONST             0xdeadbeefdeadbeefull

// This is used if the input is 96 bytes long or longer.
//
// The internal state is fully overwritten every 96 bytes.
// Every input bit appears to cause at least 128 bits of entropy
// before 96 other bytes are combined, when run forward or backward
//   For every input bit,
//   Two inputs differing in just that input bit
//   Where "differ" means xor or subtraction
//   And the base value is random
//   When run forward or backwards one Mix
// I tried 3 pairs of each; they all differed by at least 212 bits.
#define Mix(data, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11)                   \
  do {                                                                                \
    s0  += (data)[ 0]; s2  ^= s10; s11 ^= s0;  s0  = z_rotl64(s0,11);  s11 += s1;     \
    s1  += (data)[ 1]; s3  ^= s11; s0  ^= s1;  s1  = z_rotl64(s1,32);  s0  += s2;     \
    s2  += (data)[ 2]; s4  ^= s0;  s1  ^= s2;  s2  = z_rotl64(s2,43);  s1  += s3;     \
    s3  += (data)[ 3]; s5  ^= s1;  s2  ^= s3;  s3  = z_rotl64(s3,31);  s2  += s4;     \
    s4  += (data)[ 4]; s6  ^= s2;  s3  ^= s4;  s4  = z_rotl64(s4,17);  s3  += s5;     \
    s5  += (data)[ 5]; s7  ^= s3;  s4  ^= s5;  s5  = z_rotl64(s5,28);  s4  += s6;     \
    s6  += (data)[ 6]; s8  ^= s4;  s5  ^= s6;  s6  = z_rotl64(s6,39);  s5  += s7;     \
    s7  += (data)[ 7]; s9  ^= s5;  s6  ^= s7;  s7  = z_rotl64(s7,57);  s6  += s8;     \
    s8  += (data)[ 8]; s10 ^= s6;  s7  ^= s8;  s8  = z_rotl64(s8,55);  s7  += s9;     \
    s9  += (data)[ 9]; s11 ^= s7;  s8  ^= s9;  s9  = z_rotl64(s9,54);  s8  += s10;    \
    s10 += (data)[10]; s0  ^= s8;  s9  ^= s10; s10 = z_rotl64(s10,22); s9  += s11;    \
    s11 += (data)[11]; s1  ^= s9;  s10 ^= s11; s11 = z_rotl64(s11,46); s10 += s0;     \
  } while (0)

// Mix all 12 inputs together so that h0, h1 are a hash of them all.
//
// For two inputs differing in just the input bits
// Where "differ" means xor or subtraction
// And the base value is random, or a counting value starting at that bit
// The final result will have each bit of h0, h1 flip
// For every input bit,
// with probability 50 +- .3%
// For every pair of input bits,
// with probability 50 +- 3%
//
// This does not rely on the last Mix() call having already mixed some.
// Two iterations was almost good enough for a 64-bit result, but a
// 128-bit result is reported, so End() does three iterations.
#define EndPartial(h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11)   \
  do {                                                                 \
    h11+= h1;    h2 ^= h11;   h1 = z_rotl64(h1,44);        \
    h0 += h2;    h3 ^= h0;    h2 = z_rotl64(h2,15);        \
    h1 += h3;    h4 ^= h1;    h3 = z_rotl64(h3,34);        \
    h2 += h4;    h5 ^= h2;    h4 = z_rotl64(h4,21);        \
    h3 += h5;    h6 ^= h3;    h5 = z_rotl64(h5,38);        \
    h4 += h6;    h7 ^= h4;    h6 = z_rotl64(h6,33);        \
    h5 += h7;    h8 ^= h5;    h7 = z_rotl64(h7,10);        \
    h6 += h8;    h9 ^= h6;    h8 = z_rotl64(h8,13);        \
    h7 += h9;    h10^= h7;    h9 = z_rotl64(h9,38);        \
    h8 += h10;   h11^= h8;    h10= z_rotl64(h10,53);       \
    h9 += h11;   h0 ^= h9;    h11= z_rotl64(h11,42);       \
    h10+= h0;    h1 ^= h10;   h0 = z_rotl64(h0,54);        \
  } while (0)

#define End(data, h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11)           \
  do {                                                                        \
    h0 += (data)[0]; h1 += (data)[1]; h2  += (data)[2];  h3  += (data)[3];    \
    h4 += (data)[4]; h5 += (data)[5]; h6  += (data)[6];  h7  += (data)[7];    \
    h8 += (data)[8]; h9 += (data)[9]; h10 += (data)[10]; h11 += (data)[11];   \
    EndPartial(h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);                        \
    EndPartial(h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);                        \
    EndPartial(h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);                        \
  } while (0)

// The goal is for each bit of the input to expand into 128 bits of
//   apparent entropy before it is fully overwritten.
// n trials both set and cleared at least m bits of h0 h1 h2 h3
//   n: 2   m: 29
//   n: 3   m: 46
//   n: 4   m: 57
//   n: 5   m: 107
//   n: 6   m: 146
//   n: 7   m: 152
// when run forwards or backwards
// for all 1-bit and 2-bit diffs
// with diffs defined by either xor or subtraction
// with a base of all zeros plus a counter, or plus another bit, or random
#define ShortMix(h0, h1, h2, h3)                    \
  do {                                              \
    h2 = z_rotl64(h2,50);  h2 += h3;  h0 ^= h2;     \
    h3 = z_rotl64(h3,52);  h3 += h0;  h1 ^= h3;     \
    h0 = z_rotl64(h0,30);  h0 += h1;  h2 ^= h0;     \
    h1 = z_rotl64(h1,41);  h1 += h2;  h3 ^= h1;     \
    h2 = z_rotl64(h2,54);  h2 += h3;  h0 ^= h2;     \
    h3 = z_rotl64(h3,48);  h3 += h0;  h1 ^= h3;     \
    h0 = z_rotl64(h0,38);  h0 += h1;  h2 ^= h0;     \
    h1 = z_rotl64(h1,37);  h1 += h2;  h3 ^= h1;     \
    h2 = z_rotl64(h2,62);  h2 += h3;  h0 ^= h2;     \
    h3 = z_rotl64(h3,34);  h3 += h0;  h1 ^= h3;     \
    h0 = z_rotl64(h0,5);   h0 += h1;  h2 ^= h0;     \
    h1 = z_rotl64(h1,36);  h1 += h2;  h3 ^= h1;     \
  } while (0)

// Mix all 4 inputs together so that h0, h1 are a hash of them all.
//
// For two inputs differing in just the input bits
// Where "differ" means xor or subtraction
// And the base value is random, or a counting value starting at that bit
// The final result will have each bit of h0, h1 flip
// For every input bit,
// with probability 50 +- .3% (it is probably better than that)
// For every pair of input bits,
// with probability 50 +- .75% (the worst case is approximately that)
#define ShortEnd(h0, h1, h2, h3)                    \
  do {                                              \
    h3 ^= h2;  h2 = z_rotl64(h2,15);  h3 += h2;     \
    h0 ^= h3;  h3 = z_rotl64(h3,52);  h0 += h3;     \
    h1 ^= h0;  h0 = z_rotl64(h0,26);  h1 += h0;     \
    h2 ^= h1;  h1 = z_rotl64(h1,51);  h2 += h1;     \
    h3 ^= h2;  h2 = z_rotl64(h2,28);  h3 += h2;     \
    h0 ^= h3;  h3 = z_rotl64(h3,9);   h0 += h3;     \
    h1 ^= h0;  h0 = z_rotl64(h0,47);  h1 += h0;     \
    h2 ^= h1;  h1 = z_rotl64(h1,54);  h2 += h1;     \
    h3 ^= h2;  h2 = z_rotl64(h2,32);  h3 += h2;     \
    h0 ^= h3;  h3 = z_rotl64(h3,25);  h0 += h3;     \
    h1 ^= h0;  h0 = z_rotl64(h0,63);  h1 += h0;     \
  } while (0)

// short hash ... it could be used on any data,
// but it's used by Spooky just for short datas.
static void Short (const void *data, size_t size, uint64_t digest[2]) {
  uint64_t buf[24];
  union {
    const uint8_t *p8;
    uint32_t *p32;
    uint64_t *p64;
    size_t i;
  } u;

  u.p8 = (const uint8_t *)data;

  if (!ALLOW_UNALIGNED_READS && (u.i & 0x7)) {
    memcpy(buf, data, size);
    u.p64 = buf;
  }

  size_t rem = size%32;
  uint64_t a = digest[0];
  uint64_t b = digest[1];
  uint64_t c = __SC_CONST;
  uint64_t d = __SC_CONST;

  if (size > 15) {
    const uint64_t *end = u.p64 + (size/32)*4;

    // handle all complete sets of 32 bytes
    for (; u.p64 < end; u.p64 += 4) {
      c += u.p64[0];
      d += u.p64[1];
      ShortMix(a,b,c,d);
      a += u.p64[2];
      b += u.p64[3];
    }

    //Handle the case of 16+ remaining bytes.
    if (rem >= 16) {
      c += u.p64[0];
      d += u.p64[1];
      ShortMix(a,b,c,d);
      u.p64 += 2;
      rem -= 16;
    }
  }

  // Handle the last 0..15 bytes, and its size
  d += ((uint64_t)size) << 56;
  switch (rem) {
    case 15:
      d += ((uint64_t)u.p8[14]) << 48;
    case 14:
      d += ((uint64_t)u.p8[13]) << 40;
    case 13:
      d += ((uint64_t)u.p8[12]) << 32;
    case 12:
      d += u.p32[2];
      c += u.p64[0];
      break;
    case 11:
      d += ((uint64_t)u.p8[10]) << 16;
    case 10:
      d += ((uint64_t)u.p8[9]) << 8;
    case 9:
      d += (uint64_t)u.p8[8];
    case 8:
      c += u.p64[0];
      break;
    case 7:
      c += ((uint64_t)u.p8[6]) << 48;
    case 6:
      c += ((uint64_t)u.p8[5]) << 40;
    case 5:
      c += ((uint64_t)u.p8[4]) << 32;
    case 4:
      c += u.p32[0];
      break;
    case 3:
      c += ((uint64_t)u.p8[2]) << 16;
    case 2:
      c += ((uint64_t)u.p8[1]) << 8;
    case 1:
      c += (uint64_t)u.p8[0];
      break;
    case 0:
      c += __SC_CONST;
      d += __SC_CONST;
  }
  ShortEnd(a,b,c,d);
  digest[0] = a;
  digest[1] = b;
}

void z_spooky2_init (z_spooky2_t *self, uint64_t seed1, uint64_t seed2) {
  self->length = 0;
  self->rem = 0;
  self->state[0] = seed1;
  self->state[1] = seed2;
}

void z_spooky2_update (z_spooky2_t *self, const void *data, size_t size) {
  uint64_t h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11;
  size_t newsize = size + self->rem;
  uint8_t rem;
  union {
    const uint8_t *p8;
    uint64_t *p64;
    size_t i;
  } u;
  const uint64_t *end;

  // Is this data fragment too short?  If it is, stuff it away.
  if (newsize < __SC_BUFSIZE) {
    memcpy(&((uint8_t *)self->data)[self->rem], data, size);
    self->length = size + self->length;
    self->rem = (uint8_t)newsize;
    return;
  }

  // init the variables
  if (self->length < __SC_BUFSIZE) {
    h0=h3=h6=h9  = self->state[0];
    h1=h4=h7=h10 = self->state[1];
    h2=h5=h8=h11 = __SC_CONST;
  } else {
    h0 = self->state[0];
    h1 = self->state[1];
    h2 = self->state[2];
    h3 = self->state[3];
    h4 = self->state[4];
    h5 = self->state[5];
    h6 = self->state[6];
    h7 = self->state[7];
    h8 = self->state[8];
    h9 = self->state[9];
    h10 = self->state[10];
    h11 = self->state[11];
  }
  self->length = size + self->length;

  // if we've got anything stuffed away, use it now
  if (self->rem) {
    uint8_t prefix = __SC_BUFSIZE-self->rem;
    memcpy(&(((uint8_t *)self->data)[self->rem]), data, prefix);
    u.p64 = self->data;
    Mix(u.p64, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
    Mix(&u.p64[12], h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
    u.p8 = ((const uint8_t *)data) + prefix;
    size -= prefix;
  } else {
    u.p8 = (const uint8_t *)data;
  }

  // handle all whole blocks of __SC_BLOCKSIZE bytes
  end = u.p64 + (size/__SC_BLOCKSIZE)*12;
  rem = (uint8_t)(size-((const uint8_t *)end-u.p8));
  if (ALLOW_UNALIGNED_READS || (u.i & 0x7) == 0) {
    while (u.p64 < end) {
      Mix(u.p64, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
      u.p64 += 12;
    }
  } else {
    while (u.p64 < end) {
      memcpy(self->data, u.p8, __SC_BLOCKSIZE);
      Mix(self->data, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
      u.p64 += 12;
    }
  }

  // stuff away the last few bytes
  self->rem = rem;
  memcpy(self->data, end, rem);

  // stuff away the variables
  self->state[0] = h0;
  self->state[1] = h1;
  self->state[2] = h2;
  self->state[3] = h3;
  self->state[4] = h4;
  self->state[5] = h5;
  self->state[6] = h6;
  self->state[7] = h7;
  self->state[8] = h8;
  self->state[9] = h9;
  self->state[10] = h10;
  self->state[11] = h11;
}

void z_spooky2_final (z_spooky2_t *self, uint64_t digest[2]) {
  // init the variables
  if (self->length < __SC_BUFSIZE) {
    digest[0] = self->state[0];
    digest[1] = self->state[1];
    Short(self->data, self->length, digest);
    return;
  }

  const uint64_t *data = (const uint64_t *)self->data;
  uint8_t rem = self->rem;

  uint64_t h0 = self->state[0];
  uint64_t h1 = self->state[1];
  uint64_t h2 = self->state[2];
  uint64_t h3 = self->state[3];
  uint64_t h4 = self->state[4];
  uint64_t h5 = self->state[5];
  uint64_t h6 = self->state[6];
  uint64_t h7 = self->state[7];
  uint64_t h8 = self->state[8];
  uint64_t h9 = self->state[9];
  uint64_t h10 = self->state[10];
  uint64_t h11 = self->state[11];

  if (rem >= __SC_BLOCKSIZE) {
    // self->data can contain two blocks; handle any whole first block
    Mix(data, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
    data += 12;
    rem -= __SC_BLOCKSIZE;
  }

  // mix in the last partial block, and the size mod __SC_BLOCKSIZE
  memset(&((uint8_t *)data)[rem], 0, (__SC_BLOCKSIZE-rem));

  ((uint8_t *)data)[__SC_BLOCKSIZE-1] = rem;

  // do some final mixing
  End(data, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);

  digest[0] = h0;
  digest[1] = h1;
}

uint32_t z_hash32_spooky2 (uint64_t seed, const void *data, size_t size) {
  uint64_t digest[2] = { seed, seed };
  z_hash128_spooky2(data, size, digest);
  return digest[0] & 0xffffffff;
}

uint64_t z_hash64_spooky2 (uint64_t seed, const void *data, size_t size) {
  uint64_t digest[2] = { seed, seed };
  z_hash128_spooky2(data, size, digest);
  return digest[0];
}

// do the whole hash in one call
void z_hash128_spooky2 (const void *data, size_t size, uint64_t digest[2]) {
  if (size < __SC_BUFSIZE) {
    Short(data, size, digest);
    return;
  }

  uint64_t h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11;
  uint64_t buf[12];
  uint64_t *end;
  union {
      const uint8_t *p8;
      uint64_t *p64;
      size_t i;
  } u;
  size_t rem;

  h0=h3=h6=h9  = digest[1];
  h1=h4=h7=h10 = digest[1];
  h2=h5=h8=h11 = __SC_CONST;

  u.p8 = (const uint8_t *)data;
  end = u.p64 + (size/__SC_BLOCKSIZE)*12;

  // handle all whole __SC_BLOCKSIZE blocks of bytes
  if (ALLOW_UNALIGNED_READS || ((u.i & 0x7) == 0)) {
    while (u.p64 < end) {
      Mix(u.p64, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
      u.p64 += 12;
    }
  } else {
    while (u.p64 < end) {
      memcpy(buf, u.p64, __SC_BLOCKSIZE);
      Mix(buf, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
      u.p64 += 12;
    }
  }

  // handle the last partial block of __SC_BLOCKSIZE bytes
  rem = (size - ((const uint8_t *)end-(const uint8_t *)data));
  memcpy(buf, end, rem);
  memset(((uint8_t *)buf)+rem, 0, __SC_BLOCKSIZE-rem);
  ((uint8_t *)buf)[__SC_BLOCKSIZE-1] = rem;

  // do some final mixing
  End(buf, h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
  digest[0] = h0;
  digest[1] = h1;
}

void z_rand_spooky2 (uint64_t *seed, uint64_t digest[2]) {
  digest[0] = *seed;
  digest[1] = *seed;
  z_hash128_spooky2(seed, 8, digest);
  *seed += 1;
}