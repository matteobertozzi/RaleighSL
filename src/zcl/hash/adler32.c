#include <zcl/checksum.h>

uint32_t z_csum32_adler (uint32_t csum, const void *data, size_t n) {
  const unsigned char *p = (const unsigned char *)data;
  uint32_t s2 = (csum >> 16) & 0xffff;
  uint32_t s1 = (csum & 0xffff);

  while (n--) {
    s1 = (s1 + *p++) % 65521;
    s2 = (s2 + s1) % 65521;
  }

  return((s2 << 16) + s1);
}

