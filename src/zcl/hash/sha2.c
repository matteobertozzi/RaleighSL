#include <zcl/global.h>
#include <zcl/endian.h>
#include <zcl/string.h>
#include <zcl/macros.h>
#include <zcl/hash.h>
#include <zcl/bits.h>

typedef struct _SHA256_CTX {
  uint32_t  state[8];
  uint32_t  count[2];
  uint8_t buffer[64];
} SHA256_Context;


/*
 * SHA-256 checksum, as specified in FIPS 180-3, available at:
 * http://csrc.nist.gov/publications/PubsFIPS.html
 *
 * This is a very compact implementation of SHA-256.
 * It is designed to be simple and portable, not to be fast.
 */

/*
 * The literal definitions of Ch() and Maj() according to FIPS 180-3 are:
 *
 *  Ch(x, y, z)     (x & y) ^ (~x & z)
 *  Maj(x, y, z)    (x & y) ^ (x & z) ^ (y & z)
 *
 * We use equivalent logical reductions here that require one less op.
 */
#define Ch(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
#define Maj(x, y, z)  (((x) & (y)) ^ ((z) & ((x) ^ (y))))
#define Rot32(x, s) (((x) >> s) | ((x) << (32 - s)))
#define SIGMA0(x) (Rot32(x, 2) ^ Rot32(x, 13) ^ Rot32(x, 22))
#define SIGMA1(x) (Rot32(x, 6) ^ Rot32(x, 11) ^ Rot32(x, 25))
#define sigma0(x) (Rot32(x, 7) ^ Rot32(x, 18) ^ ((x) >> 3))
#define sigma1(x) (Rot32(x, 17) ^ Rot32(x, 19) ^ ((x) >> 10))

static const uint32_t SHA256_K[64] = {
  0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
  0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
  0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
  0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
  0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
  0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
  0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
  0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
  0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
  0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
  0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
  0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
  0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
  0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
  0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
  0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static void SHA256_Transform(uint32_t *H, const uint8_t *cp) {
  uint32_t a, b, c, d, e, f, g, h, t, T1, T2, W[64];

  for (t = 0; t < 16; t++, cp += 4)
    W[t] = (cp[0] << 24) | (cp[1] << 16) | (cp[2] << 8) | cp[3];

  for (t = 16; t < 64; t++)
    W[t] = sigma1(W[t - 2]) + W[t - 7] + sigma0(W[t - 15]) + W[t - 16];

  a = H[0]; b = H[1]; c = H[2]; d = H[3];
  e = H[4]; f = H[5]; g = H[6]; h = H[7];

  for (t = 0; t < 64; t++) {
    T1 = h + SIGMA1(e) + Ch(e, f, g) + SHA256_K[t] + W[t];
    T2 = SIGMA0(a) + Maj(a, b, c);
    h = g; g = f; f = e; e = d + T1;
    d = c; c = b; b = a; a = T1 + T2;
  }

  H[0] += a; H[1] += b; H[2] += c; H[3] += d;
  H[4] += e; H[5] += f; H[6] += g; H[7] += h;
}

static void SHA256_begin(SHA256_Context *context) {
  context->state[0] = 0x6a09e667UL;
  context->state[1] = 0xbb67ae85UL;
  context->state[2] = 0x3c6ef372UL;
  context->state[3] = 0xa54ff53aUL;
  context->state[4] = 0x510e527fUL;
  context->state[5] = 0x9b05688cUL;
  context->state[6] = 0x1f83d9abUL;
  context->state[7] = 0x5be0cd19UL;
  context->count[0] = 0;
  context->count[1] = 0;
}

static void SHA256_update (SHA256_Context *context,
                           const void *vdata,
                           unsigned int len)
{
  const uint8_t *data = (const uint8_t*)vdata;
  unsigned int i, j;

  j = (context->count[0] >> 3) & 63;
  if ((context->count[0] += len << 3) < (len << 3))
    context->count[1]++;
  context->count[1] += (len >> 29);

  if ((j + len) > 63) {
    z_memcpy(&context->buffer[j], data, (i = 64-j));
    SHA256_Transform(context->state, context->buffer);
    for ( ; i + 63 < len; i += 64)
      SHA256_Transform(context->state, &data[i]);
    j = 0;
  } else {
    i = 0;
  }

  z_memcpy(&context->buffer[j], &data[i], len - i);
}

static void SHA256_final(SHA256_Context *context, uint8_t digest[32]) {
  uint8_t finalcount[8];
  uint32_t i;

  for (i = 0; i < 8; i++)
    finalcount[i] = (uint8_t)((context->count[(i >= 4 ? 0 : 1)]
                                 >> ((3-(i & 3)) * 8) ) & 255);  /* Endian independent */

  SHA256_update(context, "\200", 1);
  while ((context->count[0] & 504) != 448)
    SHA256_update(context, "\0", 1);
  SHA256_update(context, finalcount, 8);  /* Should cause a SHA1Transform() */

  for (i = 0; i < 32; i++)
    digest[i] = (uint8_t)((context->state[i>>2] >> ((3-(i & 3)) * 8) ) & 255);
}

/* ============================================================================
 *  Hash256 - SHA
 */
void z_hash256_sha (uint8_t hash[32],
                    const void *data,
                    unsigned int size)
{
  SHA256_Context context;
  SHA256_begin(&context);
  SHA256_update(&context, data, size);
  SHA256_final(&context, hash);
}

/* ============================================================================
 *  Hash256 - SHA Plugin
 */
static int __sha256_init (z_hash_t *hash) {
  SHA256_Context *context;

  context = z_memory_struct_alloc(z_global_memory(), SHA256_Context);
  if (Z_MALLOC_IS_NULL(context))
    return(1);

  SHA256_begin(context);
  hash->plug_data.ptr = context;
  return(0);
}

static void __sha256_uninit (z_hash_t *hash) {
  z_memory_struct_free(z_global_memory(), SHA256_Context, hash->plug_data.ptr);
}

static void __sha256_update (z_hash_t *hash, const void *blob, unsigned int n) {
  SHA256_Context *context = (SHA256_Context *)hash->plug_data.ptr;
  SHA256_update(context, blob, n);
}

static void __sha256_digest (z_hash_t *hash, void *digest) {
  SHA256_Context *context = (SHA256_Context *)hash->plug_data.ptr;
  SHA256_final(context, digest);
}

const z_hash_plug_t z_hash256_plug_sha = {
  .init   = __sha256_init,
  .uninit = __sha256_uninit,
  .update = __sha256_update,
  .digest = __sha256_digest,
};
