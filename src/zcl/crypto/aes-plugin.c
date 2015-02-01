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

#include <zcl/crypto.h>
#include <zcl/kdf.h>

#include "codec/aes.h"

static int __aes_load (z_cryptor_t *self) {
  Z_OPAQUE_SET_PTR(&(self->data), NULL);
  return(0);
}

static int __aes_unload (z_cryptor_t *self) {
  z_aes_t *aes = Z_OPAQUE_PTR(&(self->data), z_aes_t);
  if (aes != NULL) z_aes_free(aes);
  return(0);
}

static int __aes_setkey (z_cryptor_t *self, int kd_rounds,
                         const void *key, uint32_t key_size,
                         const void *salt, uint32_t salt_size)
{
  uint8_t ikey[32];
  uint8_t iv[32];
  z_aes_t *aes;

  z_kdf(ikey, iv, kd_rounds, key, key_size, salt, salt_size);

  if ((aes = Z_OPAQUE_PTR(&(self->data), z_aes_t)) != NULL)
    z_aes_free(aes);

  aes = z_aes_alloc(ikey, iv);
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
