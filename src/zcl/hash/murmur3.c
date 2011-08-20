/*
 * http://code.google.com/p/smhasher/
 */

#include <zcl/hash.h>

#define __ROTL32(x, r)                    (((x) << (r)) | ((x) >> (32 - (r))))

uint32_t z_hash32_murmur3 (const void *blob, unsigned int n, uint32_t seed) {
    unsigned int i, nblocks;
    const uint32_t *blocks;
    const uint8_t *tail;
    const uint8_t *data;
    uint32_t h1, k1;

    data = (const uint8_t*)blob;
    nblocks = (n >> 2);

    /* body */
    h1 = seed;
    blocks = (const uint32_t *)data;
    for (i = 0; i < nblocks; ++i) {
        k1 = blocks[i];

        k1 *= 0xcc9e2d51;
        k1 = __ROTL32(k1, 15);
        k1 *= 0x1b873593;

        h1 ^= k1;
        h1 = __ROTL32(h1, 13);
        h1 = h1 * 5 + 0xe6546b64;
    }

    /* tail */
    k1 = 0;
    tail = (const uint8_t*)(data + (nblocks << 2));
    switch(n & 3) {
        case 3: k1 ^= tail[2] << 16;
        case 2: k1 ^= tail[1] << 8;
        case 1: k1 ^= tail[0];
                k1 *= 0xcc9e2d51;
                k1 = __ROTL32(k1, 15);
                k1 *= 0x1b873593;
                h1 ^= k1;
    };

    /* finalization */
    h1 ^= n;

    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;

    return(h1);
}

