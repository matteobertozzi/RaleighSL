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

static void __murmur3_process_block (z_hash32_t *hash, uint32_t k1) {
    uint32_t h1;

    k1 *= 0xcc9e2d51;
    k1 = __ROTL32(k1, 15);
    k1 *= 0x1b873593;

    h1 = hash->hash;
    h1 ^= k1;
    h1 = __ROTL32(h1, 13);
    h1 = h1 * 5 + 0xe6546b64;
    hash->hash = h1;
}

static void __murmur3_update (z_hash32_t *hash,
                              const void *blob,
                              unsigned int n)
{
    unsigned int i, nblocks;
    const uint32_t *blocks;
    const uint8_t *data;
    const uint8_t *tail;

    data = (const uint8_t *)blob;
    if ((nblocks = (4 - hash->bufsize)) != 4) {
        if (n < nblocks) {
            while (n--)
                hash->buffer[hash->bufsize++] = *data++;
            return;
        }

        n -= nblocks;
        while (nblocks--)
            hash->buffer[hash->bufsize++] = *data++;
        __murmur3_process_block(hash, Z_UINT32_PTR_VALUE(hash->buffer));
    }

    nblocks = (n >> 2);
    blocks = (const uint32_t *)data;
    for (i = 0; i < nblocks; ++i)
        __murmur3_process_block(hash, blocks[i]);

    tail = (const uint8_t*)(data + (nblocks << 2));
    switch((hash->bufsize = (n & 3))) {
        case 3: hash->buffer[2] = tail[2];
        case 2: hash->buffer[1] = tail[1];
        case 1: hash->buffer[0] = tail[0];
    }
}

static void __murmur3_digest (z_hash32_t *hash) {
    uint32_t h1, k1;

    k1 = 0;
    h1 = hash->hash;
    switch (hash->bufsize & 3) {
        case 3: k1 ^= hash->buffer[2] << 16;
        case 2: k1 ^= hash->buffer[1] << 8;
        case 1: k1 ^= hash->buffer[0];
                k1 *= 0xcc9e2d51;
                k1 = __ROTL32(k1, 15);
                k1 *= 0x1b873593;
                h1 ^= k1;
    }

    /* finalization */
    h1 ^= hash->length;

    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;

    hash->hash = h1;
}

z_hash32_plug_t z_hash32_plug_murmur3 = {
    .init   = NULL,
    .update = __murmur3_update,
    .digest = __murmur3_digest,
};

