/*
 *   Copyright 2012 Matteo Bertozzi
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#if defined(CRYPTO_COMMON_CRYPTO)

#include <CommonCrypto/CommonCryptor.h>

#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "aes.h"

struct crypto_aes {
  CCCryptorRef enc;
  CCCryptorRef dec;
  uint8_t iv[32];
};

#define __aes_cryptor_create(cryptor, op, key, iv)                          \
  (CCCryptorCreate(op, kCCAlgorithmAES128, kCCOptionPKCS7Padding,           \
                   key, kCCKeySizeAES256, iv, cryptor) != kCCSuccess)

static int __aes_process (z_aes_t *crypto,
                          CCCryptorRef cryptor,
                          const void *src, uint32_t src_size,
                          void *dst, uint32_t *dst_size)
{
  size_t out_avail;
  size_t used = 0;
  uint8_t *outp;
  size_t moved;

  outp = (uint8_t *)dst;
  out_avail = *dst_size;
  if (CCCryptorUpdate(cryptor, src, src_size, outp, out_avail, &moved)) {
    CCCryptorReset(cryptor, crypto->iv);
    return(-2);
  }

  outp += moved;
  used += moved;
  out_avail -= moved;
  if (CCCryptorFinal(cryptor, outp, out_avail, &moved)) {
    CCCryptorReset(cryptor, crypto->iv);
    return(-3);
  }

  used += moved;
  *dst_size = used;

  CCCryptorReset(cryptor, crypto->iv);
  return(0);
}

z_aes_t *z_aes_alloc (const void *key, uint32_t key_size,
                      const void *salt, uint32_t salt_size)
{
  uint8_t ikey[32];
  z_aes_t *crypto;

  /* Allocate Crypto AES Object */
  if ((crypto = (z_aes_t *) malloc(sizeof(z_aes_t))) == NULL)
    return(NULL);

  /* Key generation */
  z_aes_key(ikey, crypto->iv, key, key_size, salt, salt_size);

  /* Initialize Encryption */
  if (__aes_cryptor_create(&(crypto->enc), kCCEncrypt, ikey, crypto->iv)) {
    free(crypto);
    return(NULL);
  }

  /* Initialize Decryption */
  if (__aes_cryptor_create(&(crypto->dec), kCCDecrypt, ikey, crypto->iv)) {
    CCCryptorRelease(crypto->enc);
    free(crypto);
    return(NULL);
  }

  return(crypto);
}

void z_aes_free (z_aes_t *crypto) {
  CCCryptorRelease(crypto->enc);
  CCCryptorRelease(crypto->dec);
  free(crypto);
}

uint32_t z_aes_outlen (z_aes_t *crypto, uint32_t src_size) {
  return CCCryptorGetOutputLength(crypto->enc, src_size, 1);
}

int z_aes_encrypt (z_aes_t *crypto,
                   const void *src, uint32_t src_size,
                   void *dst, uint32_t *dst_size)
{
  return(__aes_process(crypto, crypto->enc, src, src_size, dst, dst_size));
}

int z_aes_decrypt (z_aes_t *crypto,
                   const void *src, uint32_t src_size,
                   void *dst, uint32_t *dst_size)
{
  return(__aes_process(crypto, crypto->dec, src, src_size, dst, dst_size));
}

#endif /* CRYPTO_COMMON_CRYPTO */
