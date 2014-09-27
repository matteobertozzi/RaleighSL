/*
 * https://code.google.com/p/smhasher/source/browse/trunk/MurmurHash3.cpp
 */

#include <zcl/murmur3.h>
#include <zcl/endian.h>
#include <zcl/bits.h>

//-----------------------------------------------------------------------------
// Finalization mix - force all bits of a hash block to avalanche

#define __fmix32(h)       \
  do {                    \
    h ^= h >> 16;         \
    h *= 0x85ebca6b;      \
    h ^= h >> 13;         \
    h *= 0xc2b2ae35;      \
    h ^= h >> 16;         \
  } while (0)

#define __fmix64(k)                 \
  do {                              \
    k ^= k >> 33;                   \
    k *= 0xff51afd7ed558ccdull;     \
    k ^= k >> 33;                   \
    k *= 0xc4ceb9fe1a85ec53ull;     \
    k ^= k >> 33;                   \
  } while (0)

//-----------------------------------------------------------------------------

uint32_t z_hash32_murmur3 (uint32_t seed, const void * key, size_t len) {
  const uint8_t * data = (const uint8_t*)key;
  const int nblocks = len / 4;
  int i;

  uint32_t h1 = seed;

  const uint32_t c1 = 0xcc9e2d51;
  const uint32_t c2 = 0x1b873593;

  //----------
  // body
  const uint32_t *blocks = (const uint32_t *)(data + nblocks*4);

  for (i = -nblocks; i; i++) {
    uint32_t k1;
    z_get_uint32_le(k1,blocks,i);

    k1 *= c1;
    k1 = z_rotl32(k1,15);
    k1 *= c2;

    h1 ^= k1;
    h1 = z_rotl32(h1,13);
    h1 = h1*5+0xe6546b64;
  }

  //----------
  // tail
  const uint8_t * tail = (const uint8_t*)(data + nblocks*4);
  uint32_t k1 = 0;

  switch (len & 3) {
    case 3: k1 ^= tail[2] << 16;
    case 2: k1 ^= tail[1] << 8;
    case 1: k1 ^= tail[0];
            k1 *= c1; k1 = z_rotl32(k1,15); k1 *= c2; h1 ^= k1;
  };

  //----------
  // finalization
  h1 ^= len;
  __fmix32(h1);
  return h1;
}

uint64_t z_hash64_murmur3 (uint64_t seed, const void * key, const size_t len) {
  uint64_t digest[2] = { seed, seed };
  z_hash128_murmur3(key, len, digest);
  return digest[0];
}

void z_hash128_murmur3 (const void * key, const size_t len, uint64_t digest[2]) {
  const uint8_t * data = (const uint8_t*)key;
  const int nblocks = len / 16;
  int i;

  uint64_t h1 = digest[0];
  uint64_t h2 = digest[0];

  const uint64_t c1 = 0x87c37b91114253d5ull;
  const uint64_t c2 = 0x4cf5ad432745937full;

  //----------
  // body
  const uint64_t *blocks = (const uint64_t *)(data);

  for (i = 0; i < nblocks; i++) {
    uint64_t k1, k2;

    z_get_uint32_le(k1,blocks,i*2+0);
    z_get_uint32_le(k2, blocks,i*2+1);

    k1 *= c1; k1  = z_rotl64(k1,31); k1 *= c2; h1 ^= k1;
    h1 = z_rotl64(h1,27); h1 += h2; h1 = h1*5+0x52dce729;

    k2 *= c2; k2  = z_rotl64(k2,33); k2 *= c1; h2 ^= k2;
    h2 = z_rotl64(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;
  }

  //----------
  // tail
  const uint8_t * tail = (const uint8_t*)(data + nblocks*16);

  uint64_t k1 = 0;
  uint64_t k2 = 0;

  switch (len & 15) {
    case 15: k2 ^= (uint64_t)(tail[14]) << 48;
    case 14: k2 ^= (uint64_t)(tail[13]) << 40;
    case 13: k2 ^= (uint64_t)(tail[12]) << 32;
    case 12: k2 ^= (uint64_t)(tail[11]) << 24;
    case 11: k2 ^= (uint64_t)(tail[10]) << 16;
    case 10: k2 ^= (uint64_t)(tail[ 9]) << 8;
    case  9: k2 ^= (uint64_t)(tail[ 8]) << 0;
             k2 *= c2; k2  = z_rotl64(k2,33); k2 *= c1; h2 ^= k2;

    case  8: k1 ^= (uint64_t)(tail[ 7]) << 56;
    case  7: k1 ^= (uint64_t)(tail[ 6]) << 48;
    case  6: k1 ^= (uint64_t)(tail[ 5]) << 40;
    case  5: k1 ^= (uint64_t)(tail[ 4]) << 32;
    case  4: k1 ^= (uint64_t)(tail[ 3]) << 24;
    case  3: k1 ^= (uint64_t)(tail[ 2]) << 16;
    case  2: k1 ^= (uint64_t)(tail[ 1]) << 8;
    case  1: k1 ^= (uint64_t)(tail[ 0]) << 0;
             k1 *= c1; k1  = z_rotl64(k1,31); k1 *= c2; h1 ^= k1;
  };

  //----------
  // finalization
  h1 ^= len; h2 ^= len;

  h1 += h2;
  h2 += h1;

  __fmix64(h1);
  __fmix64(h2);

  h1 += h2;
  h2 += h1;

  digest[0] = h1;
  digest[1] = h2;
}