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

#ifndef _Z_AES_H_
#define _Z_AES_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/opaque.h>

typedef struct crypto_aes z_aes_t;

void      z_aes_key     (uint8_t ikey[32], uint8_t iv[32],
                         const void *key, uint32_t key_size,
                         const void *salt, uint32_t salt_size);

z_aes_t * z_aes_alloc   (const void *key, uint32_t key_size,
                         const void *salt, uint32_t salt_size);
void      z_aes_free    (z_aes_t *crypto);

uint32_t  z_aes_outlen  (z_aes_t *crypto, uint32_t src_size);
int       z_aes_encrypt (z_aes_t *crypto,
                         const void *src, uint32_t src_size,
                         void *dst, uint32_t *dst_size);
int       z_aes_decrypt (z_aes_t *crypto,
                         const void *src, uint32_t src_size,
                         void *dst, uint32_t *dst_size);

#endif /* !_Z_AES_H_ */
