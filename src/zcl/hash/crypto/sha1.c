#include <zcl/endian.h>
#include <zcl/sha1.h>

#include <string.h>

static void __sha1_process (z_sha1_t *ctx, const uint8_t data[64]) {
  uint32_t temp, W[16], A, B, C, D, E;

  z_get_uint32_be(W[ 0], data,  0);
  z_get_uint32_be(W[ 1], data,  4);
  z_get_uint32_be(W[ 2], data,  8);
  z_get_uint32_be(W[ 3], data, 12);
  z_get_uint32_be(W[ 4], data, 16);
  z_get_uint32_be(W[ 5], data, 20);
  z_get_uint32_be(W[ 6], data, 24);
  z_get_uint32_be(W[ 7], data, 28);
  z_get_uint32_be(W[ 8], data, 32);
  z_get_uint32_be(W[ 9], data, 36);
  z_get_uint32_be(W[10], data, 40);
  z_get_uint32_be(W[11], data, 44);
  z_get_uint32_be(W[12], data, 48);
  z_get_uint32_be(W[13], data, 52);
  z_get_uint32_be(W[14], data, 56);
  z_get_uint32_be(W[15], data, 60);

#define S(x,n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#define R(t) (                                      \
  temp = W[(t -  3) & 0x0F] ^ W[(t - 8) & 0x0F] ^   \
         W[(t - 14) & 0x0F] ^ W[  t       & 0x0F],  \
  (W[t & 0x0F] = S(temp,1))                         \
)

#define P(a,b,c,d,e,x) {                            \
  e += S(a,5) + F(b,c,d) + K + x; b = S(b,30);      \
}

  A = ctx->state[0];
  B = ctx->state[1];
  C = ctx->state[2];
  D = ctx->state[3];
  E = ctx->state[4];

#define F(x,y,z) (z ^ (x & (y ^ z)))
#define K 0x5A827999
  P(A, B, C, D, E, W[0] );
  P(E, A, B, C, D, W[1] );
  P(D, E, A, B, C, W[2] );
  P(C, D, E, A, B, W[3] );
  P(B, C, D, E, A, W[4] );
  P(A, B, C, D, E, W[5] );
  P(E, A, B, C, D, W[6] );
  P(D, E, A, B, C, W[7] );
  P(C, D, E, A, B, W[8] );
  P(B, C, D, E, A, W[9] );
  P(A, B, C, D, E, W[10]);
  P(E, A, B, C, D, W[11]);
  P(D, E, A, B, C, W[12]);
  P(C, D, E, A, B, W[13]);
  P(B, C, D, E, A, W[14]);
  P(A, B, C, D, E, W[15]);
  P(E, A, B, C, D, R(16));
  P(D, E, A, B, C, R(17));
  P(C, D, E, A, B, R(18));
  P(B, C, D, E, A, R(19));
#undef K
#undef F

#define F(x,y,z) (x ^ y ^ z)
#define K 0x6ED9EBA1
  P(A, B, C, D, E, R(20));
  P(E, A, B, C, D, R(21));
  P(D, E, A, B, C, R(22));
  P(C, D, E, A, B, R(23));
  P(B, C, D, E, A, R(24));
  P(A, B, C, D, E, R(25));
  P(E, A, B, C, D, R(26));
  P(D, E, A, B, C, R(27));
  P(C, D, E, A, B, R(28));
  P(B, C, D, E, A, R(29));
  P(A, B, C, D, E, R(30));
  P(E, A, B, C, D, R(31));
  P(D, E, A, B, C, R(32));
  P(C, D, E, A, B, R(33));
  P(B, C, D, E, A, R(34));
  P(A, B, C, D, E, R(35));
  P(E, A, B, C, D, R(36));
  P(D, E, A, B, C, R(37));
  P(C, D, E, A, B, R(38));
  P(B, C, D, E, A, R(39));
#undef K
#undef F

#define F(x,y,z) ((x & y) | (z & (x | y)))
#define K 0x8F1BBCDC
  P(A, B, C, D, E, R(40));
  P(E, A, B, C, D, R(41));
  P(D, E, A, B, C, R(42));
  P(C, D, E, A, B, R(43));
  P(B, C, D, E, A, R(44));
  P(A, B, C, D, E, R(45));
  P(E, A, B, C, D, R(46));
  P(D, E, A, B, C, R(47));
  P(C, D, E, A, B, R(48));
  P(B, C, D, E, A, R(49));
  P(A, B, C, D, E, R(50));
  P(E, A, B, C, D, R(51));
  P(D, E, A, B, C, R(52));
  P(C, D, E, A, B, R(53));
  P(B, C, D, E, A, R(54));
  P(A, B, C, D, E, R(55));
  P(E, A, B, C, D, R(56));
  P(D, E, A, B, C, R(57));
  P(C, D, E, A, B, R(58));
  P(B, C, D, E, A, R(59));

#undef K
#undef F

#define F(x,y,z) (x ^ y ^ z)
#define K 0xCA62C1D6
  P(A, B, C, D, E, R(60));
  P(E, A, B, C, D, R(61));
  P(D, E, A, B, C, R(62));
  P(C, D, E, A, B, R(63));
  P(B, C, D, E, A, R(64));
  P(A, B, C, D, E, R(65));
  P(E, A, B, C, D, R(66));
  P(D, E, A, B, C, R(67));
  P(C, D, E, A, B, R(68));
  P(B, C, D, E, A, R(69));
  P(A, B, C, D, E, R(70));
  P(E, A, B, C, D, R(71));
  P(D, E, A, B, C, R(72));
  P(C, D, E, A, B, R(73));
  P(B, C, D, E, A, R(74));
  P(A, B, C, D, E, R(75));
  P(E, A, B, C, D, R(76));
  P(D, E, A, B, C, R(77));
  P(C, D, E, A, B, R(78));
  P(B, C, D, E, A, R(79));
#undef K
#undef F

  ctx->state[0] += A;
  ctx->state[1] += B;
  ctx->state[2] += C;
  ctx->state[3] += D;
  ctx->state[4] += E;
}

void z_sha1_init (z_sha1_t *ctx) {
  memset(ctx, 0, sizeof(z_sha1_t));

  ctx->total[0] = 0;
  ctx->total[1] = 0;

  ctx->state[0] = 0x67452301;
  ctx->state[1] = 0xEFCDAB89;
  ctx->state[2] = 0x98BADCFE;
  ctx->state[3] = 0x10325476;
  ctx->state[4] = 0xC3D2E1F0;
}

void z_sha1_update (z_sha1_t *ctx, const void *data, size_t ilen) {
  const uint8_t *input = (const uint8_t *)data;
  uint32_t left;
  size_t fill;

  if (ilen == 0)
    return;

  left = ctx->total[0] & 0x3F;
  fill = 64 - left;

  ctx->total[0] += ilen;
  ctx->total[0] &= 0xFFFFFFFF;

  if (ctx->total[0] < ilen)
    ctx->total[1]++;

  if (left && ilen >= fill) {
    memcpy(ctx->buffer + left, input, fill);
    __sha1_process(ctx, ctx->buffer);
    input += fill;
    ilen  -= fill;
    left = 0;
  }

  while (ilen >= 64) {
    __sha1_process(ctx, input);
    input += 64;
    ilen  -= 64;
  }

  if (ilen > 0) {
    memcpy((ctx->buffer + left), input, ilen);
  }
}

static const uint8_t __PADDING[64] = {
  0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void z_sha1_final (z_sha1_t *ctx, uint8_t output[20]) {
  uint32_t last, padn;
  uint32_t high, low;
  uint8_t msglen[8];

  high = (ctx->total[0] >> 29)
       | (ctx->total[1] <<  3);
  low  = (ctx->total[0] <<  3);

  z_put_uint32_be(high, msglen, 0);
  z_put_uint32_be(low,  msglen, 4);

  last = ctx->total[0] & 0x3F;
  padn = (last < 56) ? (56 - last) : (120 - last);

  z_sha1_update(ctx, __PADDING, padn);
  z_sha1_update(ctx, msglen, 8);

  z_put_uint32_be(ctx->state[0], output,  0);
  z_put_uint32_be(ctx->state[1], output,  4);
  z_put_uint32_be(ctx->state[2], output,  8);
  z_put_uint32_be(ctx->state[3], output, 12);
  z_put_uint32_be(ctx->state[4], output, 16);
}

void z_rand_sha1 (uint64_t *seed, uint8_t digest[20]) {
  z_sha1_t ctx;
  z_sha1_init(&ctx);
  z_sha1_update(&ctx, seed, 8);
  z_sha1_final(&ctx, digest);
  *seed += 1;
}
