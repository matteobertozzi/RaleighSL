/*
 * One-at-a-Time
 * http://www.burtleburtle.net/bob/hash/doobs.html
 */

#include <zcl/hash.h>

uint32_t z_hash32_jenkin (const void *data, unsigned int n, uint32_t seed) {
    uint32_t hash = seed;
    const uint8_t *p;

    for (p = (const uint8_t *)data; n--; ++p) {
        hash += *p;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return(hash);
}

static void __jenkin_update (z_hash32_t *hash,
                             const void *blob,
                             unsigned int n)
{
    const uint8_t *p;
    uint32_t h;

    h = hash->hash;
    for (p = (const uint8_t *)blob; n--; ++p) {
        h += *p;
        h += (h << 10);
        h ^= (h >> 6);
    }

    hash->hash = h;
}

static void __jenkin_digest (z_hash32_t *hash) {
    uint32_t h;

    h = hash->hash;
    h += (h << 3);
    h ^= (h >> 11);
    h += (h << 15);
    hash->hash = h;
}

z_hash32_plug_t z_hash32_plug_jenkin = {
    .init   = NULL,
    .update = __jenkin_update,
    .digest = __jenkin_digest,
};

