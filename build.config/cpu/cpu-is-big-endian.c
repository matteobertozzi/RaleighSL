#include <stdint.h>

/* return 0 for big endian, 1 for little endian */
int main (int argc, char **argv) {
  uint32_t x = 0x01234567;
  return((*((const uint8_t *)(&x))) == 0x67);
}
