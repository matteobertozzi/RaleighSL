#include <zcl/hash.h>

/* http://code.google.com/p/smhasher/ */
uint32_t z_hash32_murmur2 (const void *data,
                           unsigned int n,
                           uint32_t seed)
{
    const uint32_t m = 0x5bd1e995;
    const uint32_t r = 24;
    const uint32_t *p;
    const uint8_t *pc;
    uint32_t hash;
    uint32_t k;

    p = (const uint32_t *)data;
    hash = seed ^ n;
    while (n >= 4) {
        k = *p++;

        k *= m;
        k ^= (k >> r);
        k *= m;

        hash *= m;
        hash ^= k;

        n -= 4;
    }

    pc = (const uint8_t *)p;
    switch (n) {
        case 3: hash ^= pc[2] << 16;
        case 2: hash ^= pc[1] << 8;
        case 1: hash ^= pc[0];
                hash ^= m;
    }

    hash ^= hash >> 13;
    hash *= m;
    hash ^= hash >> 15;

    return(hash);
}
