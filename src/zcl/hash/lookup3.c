/*
 * http://burtleburtle.net/bob/c/lookup3.c
 * http://burtleburtle.net/bob/hash/doobs.html
 * lookup3.c, by Bob Jenkins, May 2006, Public Domain.
 */

#include <zcl/memcpy.h>
#include <zcl/hash.h>

#define rot(x, k)                               z_rotl32(x, k)

#define __lookup3_mix(a,b,c)                                                \
    a -= c;  a ^= rot(c, 4);  c += b;                                       \
    b -= a;  b ^= rot(a, 6);  a += c;                                       \
    c -= b;  c ^= rot(b, 8);  b += a;                                       \
    a -= c;  a ^= rot(c,16);  c += b;                                       \
    b -= a;  b ^= rot(a,19);  a += c;                                       \
    c -= b;  c ^= rot(b, 4);  b += a;

#define __lookup3_fetch_last12(a, b, c)                                     \
    case 12: c += k[2]; b += k[1]; a += k[0]; break;                        \
    case 11: c += k[2] & 0xffffff; b += k[1]; a += k[0]; break;             \
    case 10: c += k[2] & 0xffff; b += k[1]; a += k[0]; break;               \
    case 9 : c += k[2] & 0xff; b += k[1]; a += k[0]; break;                 \
    case 8 : b += k[1]; a += k[0]; break;                                   \
    case 7 : b += k[1] & 0xffffff; a += k[0]; break;                        \
    case 6 : b += k[1] & 0xffff; a += k[0]; break;                          \
    case 5 : b += k[1] & 0xff; a += k[0]; break;                            \
    case 4 : a += k[0]; break;                                              \
    case 3 : a += k[0] & 0xffffff; break;                                   \
    case 2 : a += k[0] & 0xffff; break;                                     \
    case 1 : a += k[0] & 0xff; break;

#define __lookup3_final(a,b,c)                                              \
    c ^= b; c -= rot(b,14);                                                 \
    a ^= c; a -= rot(c,11);                                                 \
    b ^= a; b -= rot(a,25);                                                 \
    c ^= b; c -= rot(b,16);                                                 \
    a ^= c; a -= rot(c,4);                                                  \
    b ^= a; b -= rot(a,14);                                                 \
    c ^= b; c -= rot(b,24);


uint32_t z_hash32_lookup3 (const void *blob, unsigned int n, uint32_t seed) {
    const uint32_t *k;
    uint32_t a, b, c;

    /* Set up the internal state */
    a = b = c = 0xdeadbeef + seed;

    k = (const uint32_t *)blob;

    /*----------------------------------------------- handle most of the key */
    while (n > 12) {
        a += k[0];
        b += k[1];
        c += k[2];
        __lookup3_mix(a, b, c);

        n -= 12;
        k += 3;
    }

    /*----------------------------------------- handle the last 3 uint32_t's */
    switch (n) {
        __lookup3_fetch_last12(a, b, c);

        /* zero length strings require no mixing */
        case 0 : return(c);
    }

    __lookup3_final(a, b, c);
    return(c);
}

/* ============================================================================
 *  Hash32 Plugin
 */
void __lookup3_process_block (z_hash32_t *hash, uint32_t *blocks) {
    uint32_t *pa, *pb, *pc;
    uint32_t a, b, c;

    pa = (uint32_t *)(hash->buffer + 16);
    pb = (uint32_t *)(hash->buffer + 20);
    pc = (uint32_t *)(hash->buffer + 24);

    a = *pa;
    b = *pb;
    c = *pc;

    a += blocks[0];
    b += blocks[1];
    c += blocks[2];

   __lookup3_mix(a, b, c);

    *pa = a;
    *pb = b;
    *pc = c;
}

static void __lookup3_init (z_hash32_t *hash, uint32_t seed) {
    uint32_t *a, *b, *c;

    a = Z_UINT32_PTR(hash->buffer + 16);
    b = Z_UINT32_PTR(hash->buffer + 20);
    c = Z_UINT32_PTR(hash->buffer + 24);

    *a = *b = *c = 0xdeadbeef + seed;
}

static void __lookup3_update (z_hash32_t *hash,
                              const void *blob,
                              unsigned int n)
{
    unsigned int nblocks;
    const uint8_t *data;

    data = (const uint8_t *)blob;
    if ((nblocks = (12 - hash->bufsize)) > 0) {
        if (n < nblocks) {
            z_memcpy(hash->buffer + hash->bufsize, blob, n);
            hash->bufsize += n;
            return;
        }

        n -= nblocks;
        data += nblocks;
        z_memcpy(hash->buffer + hash->bufsize, blob, nblocks);
        __lookup3_process_block(hash, Z_UINT32_PTR(hash->buffer));
        hash->bufsize = 0;
    } else if (n > 0) {
        __lookup3_process_block(hash, Z_UINT32_PTR(hash->buffer));
    }

    while (n > 12) {
        __lookup3_process_block(hash, Z_UINT32_PTR(data));
        data += 12;
        n -= 12;
    }

    z_memcpy(hash->buffer, data, n);
    hash->bufsize = n;
}

static void __lookup3_digest (z_hash32_t *hash) {
    const uint32_t *k;
    uint32_t a, b, c;

    a = Z_UINT32_PTR_VALUE(hash->buffer + 16);
    b = Z_UINT32_PTR_VALUE(hash->buffer + 20);
    c = Z_UINT32_PTR_VALUE(hash->buffer + 24);

    k = Z_UINT32_PTR(hash->buffer);
    switch (hash->bufsize) {
        __lookup3_fetch_last12(a, b, c);

        /* zero length strings require no mixing */
        case 0 :
            hash->hash = c;
            return;
    }

    __lookup3_final(a, b, c);
    hash->hash = c;
}

z_hash32_plug_t z_hash32_plug_lookup3 = {
    .init   = __lookup3_init,
    .update = __lookup3_update,
    .digest = __lookup3_digest,
};

