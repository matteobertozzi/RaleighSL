#include <stdint.h>
#include <stdio.h>
/* return 0 for big endian, 1 for little endian */
int main (int argc, char **argv) {
  // 0b1001000110100010101100111
  uint32_t x = 0x01234567;
  const uint8_t *px = ((const uint8_t *)&x);
  fprintf(stderr, "%x\n", px[0]);
  fprintf(stderr, "%x\n", px[1]);
  fprintf(stderr, "%x\n", px[2]);
  fprintf(stderr, "%x\n", px[3]);
  return(!(px[0] == 0x91));
}
