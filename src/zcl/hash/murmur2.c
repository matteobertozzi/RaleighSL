/*
 * http://code.google.com/p/smhasher/
 */

#include <zcl/hash.h>

uint32_t z_hash32_murmur2 (const void *data, unsigned int n, uint32_t seed) {
    const uint32_t *p;
    const uint8_t *pc;
    uint32_t hash;
    uint32_t k;

    p = (const uint32_t *)data;
    hash = seed ^ n;
    while (n >= 4) {
        k = *p++;

        k *= 0x5bd1e995;
        k ^= (k >> 24);
        k *= 0x5bd1e995;

        hash *= 0x5bd1e995;
        hash ^= k;

        n -= 4;
    }

    pc = (const uint8_t *)p;
    switch (n) {
        case 3: hash ^= pc[2] << 16;
        case 2: hash ^= pc[1] << 8;
        case 1: hash ^= pc[0];
                hash ^= 0x5bd1e995;
    }

    hash ^= hash >> 13;
    hash *= 0x5bd1e995;
    hash ^= hash >> 15;

    return(hash);
}

