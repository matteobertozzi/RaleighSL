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

#include <zcl/kdf.h>

#if defined(__AES_DERIVATION_USE_SHA1)
  #include <zcl/sha1.h>
#elif defined(__AES_DERIVATION_USE_SHA256)
  #define __AES_DERIVATION_USE_SHA256        1
  #include <zcl/sha256.h>
#elif !defined(__AES_DERIVATION_USE_SHA512)
  #define __AES_DERIVATION_USE_SHA512        1
  #include <zcl/sha512.h>
#endif

#if defined(__AES_DERIVATION_USE_SHA1)
  #define __KDF_MD_SIZE           20
  #define __kdf_md_t              z_sha1_t
  #define __kdf_md_init           z_sha1_init
  #define __kdf_md_update         z_sha1_update
  #define __kdf_md_final          z_sha1_final
#elif defined(__AES_DERIVATION_USE_SHA256)
  #define __KDF_MD_SIZE           32
  #define __kdf_md_t              z_sha256_t
  #define __kdf_md_init           z_sha256_init
  #define __kdf_md_update         z_sha256_update
  #define __kdf_md_final          z_sha256_final
#elif defined(__AES_DERIVATION_USE_SHA512)
  #define __KDF_MD_SIZE           64
  #define __kdf_md_t              z_sha512_t
  #define __kdf_md_init           z_sha512_init
  #define __kdf_md_update         z_sha512_update
  #define __kdf_md_final          z_sha512_final
#else
  #error "Key Derivation MD not defined"
#endif

void z_kdf (uint8_t ikey[32], uint8_t iv[32],
            int derivation_rounds,
            const void *key, uint32_t key_size,
            const void *salt, uint32_t salt_size)
{
  uint8_t md_buf[__KDF_MD_SIZE];
  uint32_t nkey = 32;
  uint32_t niv = 32;
  __kdf_md_t md;
  int i;

  __kdf_md_init(&md);
  for (;;) {
    __kdf_md_update(&md, key, key_size);
    __kdf_md_update(&md, salt, salt_size);
    __kdf_md_final(&md, md_buf);

    for (i = 1; i < derivation_rounds; ++i) {
      __kdf_md_init(&md);
      __kdf_md_update(&md, md_buf, __KDF_MD_SIZE);
      __kdf_md_final(&md, md_buf);
    }

    i = 0;
    if (nkey) {
      for (;;) {
        if (nkey == 0 || i == __KDF_MD_SIZE)
          break;
        *(ikey++) = md_buf[i];
        nkey--;
        i++;
      }
    }

    if (niv && i != __KDF_MD_SIZE) {
      for (;;) {
        if (niv == 0 || i == __KDF_MD_SIZE)
          break;
        *(iv++) = md_buf[i];
        niv--;
        i++;
      }
    }

    if (nkey == 0 && niv == 0)
      break;

    __kdf_md_init(&md);
    __kdf_md_update(&md, md_buf, __KDF_MD_SIZE);
  }
}
