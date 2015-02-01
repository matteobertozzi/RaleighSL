#include <zcl/crypto.h>
#include <zcl/time.h>
#include <zcl/kdf.h>

#include <string.h>
#include <stdio.h>

static void __binary_dump (const uint8_t *buf, uint32_t size) {
  const uint8_t *pbuf = buf;
  while (size --> 0) {
    printf("%02x", *pbuf++);
  }
  printf("\n");
}

static int __test_crypto (const z_crypto_vtable_t *vtable) {
  z_cryptor_t cryptor;
  uint8_t input[640];
  uint8_t cbuf[1024];
  uint8_t ubuf[1024];
  uint32_t csize;
  int r;

  memset(input +   0, 10, 128);
  memset(input + 128, 20, 128);
  memset(input + 256, 30, 128);
  memset(input + 384, 40, 128);
  memset(input + 512, 50, 128);
  memset(cbuf, 1, 1024);
  memset(ubuf, 2, 1024);

  vtable->load(&cryptor);

  vtable->setkey(&cryptor, 107591, "key", 3, "salt", 4);

  printf("outlen %u\n", vtable->outlen(&cryptor, 640));

  /* run 1 */
  csize = 1024;
  r = vtable->encrypt(&cryptor, input, 640, cbuf, &csize);
  printf("encrypt r=%d csize=%u - ", r, csize);
  __binary_dump(cbuf, 30);

  r = vtable->decrypt(&cryptor, cbuf, csize, ubuf, 640);
  printf("decrypt r=%d cmp=%d\n", r, memcmp(ubuf, input, 640));

  /* run 2 */
  csize = 1024;
  r = vtable->encrypt(&cryptor, input, 640, cbuf, &csize);
  printf("encrypt r=%d csize=%u - ", r, csize);
  __binary_dump(cbuf, 30);

  r = vtable->decrypt(&cryptor, cbuf, csize, ubuf, 640);
  printf("decrypt r=%d cmp=%d\n", r, memcmp(ubuf, input, 640));

  vtable->unload(&cryptor);

  return(0);
}

int main (int argc, char **argv) {
  uint8_t ikey[32];
  uint8_t iv[32];

  uint64_t st, et;
  st = z_time_micros();
  z_kdf(ikey, iv, 107591u,
        "this is a key", 13,
        "this is a random testing salt 57", 32);
  et = z_time_micros();
  printf("Key Derivation %"PRIu64"uusec %.3fmsec %.3fsec\n",
         et - st, (et - st) / 1000.0f, (et - st) / 1000000.0f);
  printf(" - ikey: "); __binary_dump(ikey, 32);
  printf(" - iv:   "); __binary_dump(iv, 32);

  __test_crypto(&z_crypto_aes);
  return(0);
}