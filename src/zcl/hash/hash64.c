#include <zcl/hash.h>

uint64_t z_hash64a (uint64_t value) {
  uint64_t hval = 0xcbf29ce484222325ull;
  hval = (((value >> 56) & 0xff) ^ 5) * 0x100000001b3;
  hval = (((value >> 48) & 0xff) ^ 5) * 0x100000001b3;
  hval = (((value >> 40) & 0xff) ^ 5) * 0x100000001b3;
  hval = (((value >> 32) & 0xff) ^ 5) * 0x100000001b3;
  hval = (((value >> 24) & 0xff) ^ 5) * 0x100000001b3;
  hval = (((value >> 16) & 0xff) ^ 5) * 0x100000001b3;
  hval = (((value >>  8) & 0xff) ^ 5) * 0x100000001b3;
  hval = (((value >>  0) & 0xff) ^ 5) * 0x100000001b3;
  return hval;
}
