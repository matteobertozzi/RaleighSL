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

#ifndef _Z_CRYPTO_H_
#define _Z_CRYPTO_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/opaque.h>

Z_TYPEDEF_STRUCT(z_crypto_vtable)
Z_TYPEDEF_STRUCT(z_cryptor)

struct z_cryptor {
  z_opaque_t data;
};

typedef int (*z_encrypt_t)  (z_cryptor_t *self,
                             const uint8_t *src, uint32_t src_len,
                             uint8_t *dst, uint32_t *dst_len);
typedef int (*z_decrypt_t)  (z_cryptor_t *self,
                             const uint8_t *src, uint32_t src_len,
                             uint8_t *dst, uint32_t dst_len);

struct z_crypto_vtable {
  int       (*load)   (z_cryptor_t *self);
  int       (*unload) (z_cryptor_t *self);
  int       (*setkey) (z_cryptor_t *self, int kd_rounds,
                       const void *key, uint32_t key_size,
                       const void *salt, uint32_t salt_size);
  uint32_t  (*outlen) (z_cryptor_t *self, uint32_t src_len);

  z_encrypt_t encrypt;
  z_decrypt_t decrypt;
};

extern const z_crypto_vtable_t z_crypto_aes;

#endif /* !_Z_CRYPTO_H_ */
