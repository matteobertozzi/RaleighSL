/*
 * http://www.partow.net/programming/hashfunctions
 */

#include <zcl/hash.h>

/* A bitwise hash function written by Justin Sobel
 */
uint32_t z_hash32_js (const void *data, unsigned int n, uint32_t seed) {
    uint32_t hash = 1315423911;
    const uint8_t *p;

    for (p = (const uint8_t *)data; n--; p++)
        hash ^= ((hash << 5) + *p + (hash >> 2));

    return(hash);
}

/* Similar to the PJW Hash function, but tweaked for 32-bit processors.
 * Its the hash function widely used on most UNIX systems.
 */
uint32_t z_hash32_elf (const void *data, unsigned int n, uint32_t seed) {
    uint32_t hash = seed;
    const uint8_t *p;
    uint32_t x;

    for (p = (const uint8_t *)data; n--; p++) {
        hash = (hash << 4) + *p++;
        if ((x = hash & 0xf0000000) != 0)
            hash ^= (x >> 24);
        hash &= ~x;
    }

    return(hash);
}

/*
 * These functions are based on Peter J. Weinberger's hash function
 * (from the Dragon Book). The constant 24 in the original function
 * was replaced with 23 to produce fewer collisions on input such as
 * "a", "aa", "aaa", "aaaa", ...
 */
uint32_t z_hash32_elfv (const void *data, unsigned int n, uint32_t seed) {
    uint32_t hash = seed;
    const uint8_t *p;

    for (p = (const uint8_t *)data; n--; p++) {
        hash  = (hash << 4) + *p;
        hash ^= (hash & 0xf0000000) >> 23;
        hash &= 0x0fffffff;
    }

    return(hash);
}

/* This is the algorithm of choice which is used in the open source
 * SDBM project. The hash function seems to have a good over-all distribution
 * for many different data sets. It seems to work well in situations where
 * there is a high variance in the MSBs of the elements in a data set.
 */
uint32_t z_hash32_sdbm (const void *data, unsigned int n, uint32_t seed) {
    uint32_t hash = seed;
    const uint8_t *p;

    for (p = (const uint8_t *)data; n--; p++)
        hash = *p + (hash << 6) + (hash << 16) - hash;

    return(hash);
}

/* An algorithm proposed by Donald E. Knuth in The Art Of Computer
 * Programming Volume 3, under the topic of sorting and search chapter 6.4.
 */
uint32_t z_hash32_dek (const void *data, unsigned int n, uint32_t seed) {
    uint32_t hash = seed;
    const uint8_t *p;

    for (p = (const uint8_t *)data; n--; p++)
        hash = ((hash << 5) ^ (hash >> 27) ^ *p);

    return(hash);
}

uint32_t z_hash32_string (const void *data, unsigned int n, uint32_t seed) {
    uint32_t hash = seed;
    const uint8_t *p;

    for (p = (const uint8_t *)data; n--; p++)
        hash = 31 * hash + *p;

    return(hash);
}

