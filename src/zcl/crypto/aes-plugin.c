/*
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

#include <zcl/sha1.h>

#include <zcl/crypto.h>

#include "codec/aes.h"

#ifndef __AES_DERIVATION_ROUNDS
    #define __AES_DERIVATION_ROUNDS     (5U)
#endif /* !__AES_DERIVATION_ROUNDS */

void z_aes_key (uint8_t ikey[32], uint8_t iv[32],
                const void *key, uint32_t key_size,
                const void *salt, uint32_t salt_size)
{
  uint8_t sha1_buf[20];
  uint32_t nkey = 32;
  uint32_t niv = 32;
  z_sha1_t sha1;
  int i;

  z_sha1_init(&sha1);
  for (;;) {
    z_sha1_update(&sha1, key, key_size);
    z_sha1_update(&sha1, salt, salt_size);
    z_sha1_final(&sha1, sha1_buf);

    for (i = 1; i < __AES_DERIVATION_ROUNDS; ++i) {
      z_sha1_init(&sha1);
      z_sha1_update(&sha1, sha1_buf, 20);
      z_sha1_final(&sha1, sha1_buf);
    }

    i = 0;
    if (nkey > 0) {
      for (;;) {
        if (nkey == 0 || i == 20)
          break;
        *(ikey++) = sha1_buf[i];
        nkey--;
        i++;
      }
    }

    if (niv > 0 && (i != 20)) {
      for (;;) {
        if (niv == 0 || i == 20)
          break;

        *iv++ = sha1_buf[i];
        niv--;
        i++;
      }
    }

    if (nkey == 0 && niv == 0)
      break;
          
    z_sha1_init(&sha1);
    z_sha1_update(&sha1, sha1_buf, 20);
  }
}

static int __aes_load (z_cryptor_t *self) {
  Z_OPAQUE_SET_PTR(&(self->data), NULL);
  return(0);
}
  
static int __aes_unload (z_cryptor_t *self) {
  z_aes_t *aes = Z_OPAQUE_PTR(&(self->data), z_aes_t);
  if (aes != NULL) z_aes_free(aes);
  return(0);
}
  
static int __aes_setkey (z_cryptor_t *self,
                         const void *key, uint32_t key_size,
                         const void *salt, uint32_t salt_size)
{
  z_aes_t *aes = Z_OPAQUE_PTR(&(self->data), z_aes_t);
  if (aes != NULL) z_aes_free(aes);

  aes = z_aes_alloc(key, key_size, salt, salt_size);
  Z_OPAQUE_SET_PTR(&(self->data), aes);
  return(aes == NULL);
}

static uint32_t __aes_outlen (z_cryptor_t *self, uint32_t src_size) {
  return z_aes_outlen(Z_OPAQUE_PTR(&(self->data), z_aes_t), src_size);
}

static int __aes_encrypt (z_cryptor_t *self,
                          const uint8_t *src, uint32_t src_len,
                          uint8_t *dst, uint32_t *dst_len)
{
  return z_aes_encrypt(Z_OPAQUE_PTR(&(self->data), z_aes_t),
                       src, src_len, dst, dst_len);
}

static int __aes_decrypt (z_cryptor_t *self,
                          const uint8_t *src, uint32_t src_len,
                          uint8_t *dst, uint32_t dst_len)
{
  uint32_t usize = dst_len;
  int r;
  r = z_aes_decrypt(Z_OPAQUE_PTR(&(self->data), z_aes_t),
                    src, src_len, dst, &usize);
  return(r || dst_len != usize);
}

const z_crypto_vtable_t z_crypto_aes = {
  .load    = __aes_load,
  .unload  = __aes_unload,
  .setkey  = __aes_setkey,
  .outlen  = __aes_outlen,
  .encrypt = __aes_encrypt,
  .decrypt = __aes_decrypt,
};
