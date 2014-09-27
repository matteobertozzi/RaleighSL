#include <zcl/endian.h>
#include <zcl/sha512.h>

#include <string.h>

static const uint64_t K[80] = {
  0x428A2F98D728AE22, 0x7137449123EF65CD,
  0xB5C0FBCFEC4D3B2F, 0xE9B5DBA58189DBBC,
  0x3956C25BF348B538, 0x59F111F1B605D019,
  0x923F82A4AF194F9B, 0xAB1C5ED5DA6D8118,
  0xD807AA98A3030242, 0x12835B0145706FBE,
  0x243185BE4EE4B28C, 0x550C7DC3D5FFB4E2,
  0x72BE5D74F27B896F, 0x80DEB1FE3B1696B1,
  0x9BDC06A725C71235, 0xC19BF174CF692694,
  0xE49B69C19EF14AD2, 0xEFBE4786384F25E3,
  0x0FC19DC68B8CD5B5, 0x240CA1CC77AC9C65,
  0x2DE92C6F592B0275, 0x4A7484AA6EA6E483,
  0x5CB0A9DCBD41FBD4, 0x76F988DA831153B5,
  0x983E5152EE66DFAB, 0xA831C66D2DB43210,
  0xB00327C898FB213F, 0xBF597FC7BEEF0EE4,
  0xC6E00BF33DA88FC2, 0xD5A79147930AA725,
  0x06CA6351E003826F, 0x142929670A0E6E70,
  0x27B70A8546D22FFC, 0x2E1B21385C26C926,
  0x4D2C6DFC5AC42AED, 0x53380D139D95B3DF,
  0x650A73548BAF63DE, 0x766A0ABB3C77B2A8,
  0x81C2C92E47EDAEE6, 0x92722C851482353B,
  0xA2BFE8A14CF10364, 0xA81A664BBC423001,
  0xC24B8B70D0F89791, 0xC76C51A30654BE30,
  0xD192E819D6EF5218, 0xD69906245565A910,
  0xF40E35855771202A, 0x106AA07032BBD1B8,
  0x19A4C116B8D2D0C8, 0x1E376C085141AB53,
  0x2748774CDF8EEB99, 0x34B0BCB5E19B48A8,
  0x391C0CB3C5C95A63, 0x4ED8AA4AE3418ACB,
  0x5B9CCA4F7763E373, 0x682E6FF3D6B2B8A3,
  0x748F82EE5DEFB2FC, 0x78A5636F43172F60,
  0x84C87814A1F0AB72, 0x8CC702081A6439EC,
  0x90BEFFFA23631E28, 0xA4506CEBDE82BDE9,
  0xBEF9A3F7B2C67915, 0xC67178F2E372532B,
  0xCA273ECEEA26619C, 0xD186B8C721C0C207,
  0xEADA7DD6CDE0EB1E, 0xF57D4F7FEE6ED178,
  0x06F067AA72176FBA, 0x0A637DC5A2C898A6,
  0x113F9804BEF90DAE, 0x1B710B35131C471B,
  0x28DB77F523047D84, 0x32CAAB7B40C72493,
  0x3C9EBE0A15C9BEBC, 0x431D67C49C100D4C,
  0x4CC5D4BECB3E42B6, 0x597F299CFC657E2A,
  0x5FCB6FAB3AD6FAEC, 0x6C44198C4A475817,
};

static void __sha512_process (z_sha512_t *ctx, const uint8_t data[128]) {
  int i;
  uint64_t temp1, temp2, W[80];
  uint64_t A, B, C, D, E, F, G, H;

#define SHR(x,n) (x >> n)
#define ROTR(x,n) (SHR(x,n) | (x << (64 - n)))

#define S0(x) (ROTR(x, 1) ^ ROTR(x, 8) ^  SHR(x, 7))
#define S1(x) (ROTR(x,19) ^ ROTR(x,61) ^  SHR(x, 6))

#define S2(x) (ROTR(x,28) ^ ROTR(x,34) ^ ROTR(x,39))
#define S3(x) (ROTR(x,14) ^ ROTR(x,18) ^ ROTR(x,41))

#define F0(x,y,z) ((x & y) | (z & (x | y)))
#define F1(x,y,z) (z ^ (x & (y ^ z)))

#define P(a,b,c,d,e,f,g,h,x,K)                  \
  do {                                          \
    temp1 = h + S3(e) + F1(e,f,g) + K + x;      \
    temp2 = S2(a) + F0(a,b,c);                  \
    d += temp1; h = temp1 + temp2;              \
  } while (0)

  for (i = 0; i < 16; i++) {
    z_get_uint64_be(W[i], data, i << 3);
  }

  for (; i < 80; i++) {
    W[i] = S1(W[i -  2]) + W[i -  7] +
           S0(W[i - 15]) + W[i - 16];
  }

  A = ctx->state[0];
  B = ctx->state[1];
  C = ctx->state[2];
  D = ctx->state[3];
  E = ctx->state[4];
  F = ctx->state[5];
  G = ctx->state[6];
  H = ctx->state[7];
  i = 0;

  do {
    P(A, B, C, D, E, F, G, H, W[i], K[i]); i++;
    P(H, A, B, C, D, E, F, G, W[i], K[i]); i++;
    P(G, H, A, B, C, D, E, F, W[i], K[i]); i++;
    P(F, G, H, A, B, C, D, E, W[i], K[i]); i++;
    P(E, F, G, H, A, B, C, D, W[i], K[i]); i++;
    P(D, E, F, G, H, A, B, C, W[i], K[i]); i++;
    P(C, D, E, F, G, H, A, B, W[i], K[i]); i++;
    P(B, C, D, E, F, G, H, A, W[i], K[i]); i++;
  } while (i < 80);

  ctx->state[0] += A;
  ctx->state[1] += B;
  ctx->state[2] += C;
  ctx->state[3] += D;
  ctx->state[4] += E;
  ctx->state[5] += F;
  ctx->state[6] += G;
  ctx->state[7] += H;
}

void z_sha512_init (z_sha512_t *ctx) {
  memset(ctx, 0, sizeof(z_sha512_t));

  ctx->total[0] = 0;
  ctx->total[1] = 0;

  ctx->state[0] = 0x6A09E667F3BCC908;
  ctx->state[1] = 0xBB67AE8584CAA73B;
  ctx->state[2] = 0x3C6EF372FE94F82B;
  ctx->state[3] = 0xA54FF53A5F1D36F1;
  ctx->state[4] = 0x510E527FADE682D1;
  ctx->state[5] = 0x9B05688C2B3E6C1F;
  ctx->state[6] = 0x1F83D9ABFB41BD6B;
  ctx->state[7] = 0x5BE0CD19137E2179;
}

void z_sha512_update (z_sha512_t *ctx, const void *data, size_t ilen) {
  const uint8_t *input = (const uint8_t *)data;
  uint32_t left;
  size_t fill;

  if (ilen == 0)
    return;

  left = ctx->total[0] & 0x7F;
  fill = 128 - left;

  ctx->total[0] += ilen;

  if (ctx->total[0] < ilen)
    ctx->total[1]++;

  if (left && ilen >= fill) {
    memcpy(ctx->buffer + left, input, fill);
    __sha512_process(ctx, ctx->buffer);
    input += fill;
    ilen  -= fill;
    left = 0;
  }

  while (ilen >= 128) {
    __sha512_process(ctx, input);
    input += 128;
    ilen  -= 128;
  }

  if (ilen > 0) {
    memcpy((ctx->buffer + left), input, ilen);
  }
}

static const uint8_t __PADDING[128] = {
  0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void z_sha512_final (z_sha512_t *ctx, uint8_t output[64]) {
  size_t last, padn;
  uint64_t high, low;
  uint8_t msglen[16];

  high = (ctx->total[0] >> 61)
       | (ctx->total[1] <<  3);
  low  = (ctx->total[0] <<  3);

  z_put_uint64_be(high, msglen, 0);
  z_put_uint64_be(low,  msglen, 8);

  last = (size_t)(ctx->total[0] & 0x7F);
  padn = (last < 112) ? (112 - last) : (240 - last);

  z_sha512_update(ctx, __PADDING, padn);
  z_sha512_update(ctx, msglen, 16);

  z_put_uint64_be(ctx->state[0], output,  0);
  z_put_uint64_be(ctx->state[1], output,  8);
  z_put_uint64_be(ctx->state[2], output, 16);
  z_put_uint64_be(ctx->state[3], output, 24);
  z_put_uint64_be(ctx->state[4], output, 32);
  z_put_uint64_be(ctx->state[5], output, 40);
  z_put_uint64_be(ctx->state[6], output, 48);
  z_put_uint64_be(ctx->state[7], output, 56);
}

void z_rand_sha512 (uint64_t *seed, uint8_t digest[64]) {
  z_sha512_t ctx;
  z_sha512_init(&ctx);
  z_sha512_update(&ctx, seed, 8);
  z_sha512_final(&ctx, digest);
  *seed += 1;
}
