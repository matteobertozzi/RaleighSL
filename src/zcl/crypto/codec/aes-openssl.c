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

#if defined(CRYPTO_OPENSSL)

#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <openssl/aes.h>
#include <openssl/evp.h>

#include "aes.h"

struct crypto_aes {
  EVP_CIPHER_CTX enc;
  EVP_CIPHER_CTX dec;
};

z_aes_t *z_aes_alloc (const void *key, uint32_t key_size,
                      const void *salt, uint32_t salt_size)
{
  unsigned char ikey[32];
  unsigned char iv[32];
  z_aes_t *crypto;

  /* Key Derivation */
  z_aes_key(ikey, iv, key, key_size, salt, salt_size);

  /* Allocate Crypto AES Object */
  if ((crypto = (z_aes_t *) malloc(sizeof(z_aes_t))) == NULL)
    return(NULL);

  /* Initialize Encryption */
  EVP_CIPHER_CTX_init(&(crypto->enc));
  if (!EVP_EncryptInit_ex(&(crypto->enc), EVP_aes_256_cbc(), NULL, ikey, iv)) {
    EVP_CIPHER_CTX_cleanup(&(crypto->enc));
    free(crypto);
    return(NULL);
  }

  /* Initialize Decryption */
  EVP_CIPHER_CTX_init(&(crypto->dec));
  if (!EVP_DecryptInit_ex(&(crypto->dec), EVP_aes_256_cbc(), NULL, ikey, iv)) {
    EVP_CIPHER_CTX_cleanup(&(crypto->enc));
    EVP_CIPHER_CTX_cleanup(&(crypto->dec));
    free(crypto);
    return(NULL);
  }

  return(crypto);
}

void z_aes_free (z_aes_t *crypto) {
  EVP_CIPHER_CTX_cleanup(&(crypto->enc));
  EVP_CIPHER_CTX_cleanup(&(crypto->dec));
  free(crypto);
}

uint32_t z_aes_outlen (z_aes_t *crypto, uint32_t src_size) {
  uint32_t blksize;
  blksize = EVP_CIPHER_CTX_block_size(&(crypto->enc));
  return(src_size + blksize - (src_size % blksize));
}

int z_aes_encrypt (z_aes_t *crypto,
                   const void *src, uint32_t src_size,
                   void *dst, uint32_t *dst_size)
{
  EVP_CIPHER_CTX *e = &(crypto->enc);
  int psize = 0;
  int fsize = 0;

  /* allows reusing of 'e' for multiple encryption cycles */
  if (!EVP_EncryptInit_ex(e, NULL, NULL, NULL, NULL))
    return(-1);

  /* update ciphertext, c_len is filled with the length of ciphertext
   * generated, *len is the size of plaintext in bytes
   */
  if (!EVP_EncryptUpdate(e, dst, &psize, src, src_size))
    return(-2);

  /* update ciphertext with the final remaining bytes */
  if (!EVP_EncryptFinal_ex(e, dst + psize, &fsize)) {
    return(-3);
  }

  *dst_size = psize + fsize;
  return(0);
}

int z_aes_decrypt (z_aes_t *crypto,
                   const void *src, uint32_t src_size,
                   void *dst, uint32_t *dst_size)
{
  EVP_CIPHER_CTX *e = &(crypto->dec);
  int psize = 0;
  int fsize = 0;

  if (!EVP_DecryptInit_ex(e, NULL, NULL, NULL, NULL))
    return(-1);

  if (!EVP_DecryptUpdate(e, dst, &psize, src, src_size))
    return(-2);

  if (!EVP_DecryptFinal_ex(e, dst + psize, &fsize))
    return(-3);

  *dst_size = psize + fsize;
  return(0);
}

#endif /* CRYPTO_OPENSSL */

