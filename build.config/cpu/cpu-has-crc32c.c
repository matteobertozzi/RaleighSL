#include <stdint.h>
#include <stdio.h>

int main (int argc, char **argv) {
    uint64_t u64 = 18446744073709551610ULL;
    uint32_t u32 = 4294967290UL;
    uint16_t u16 = 65530;
    uint8_t u8 = 250;
    uint32_t crc = 7;

    if ((crc = __builtin_ia32_crc32qi(crc, u8)) != 1279665062UL)
        return(1);

    if ((crc = __builtin_ia32_crc32hi(crc, u16)) != 2380068828UL)
        return(2);

    if ((crc = __builtin_ia32_crc32si(crc, u32)) != 4043200349UL)
        return(3);

    if ((crc = __builtin_ia32_crc32di(crc, u64)) != 1895390231)
        return(4);

    return(0);
}
