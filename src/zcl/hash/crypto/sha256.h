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

#ifndef _Z_SHA256_H_
#define _Z_SHA256_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/debug.h>

typedef struct z_sha256 {
  uint32_t total[2];
  uint32_t state[8];
  uint8_t  buffer[64];
} z_sha256_t;

void z_sha256_init   (z_sha256_t *self);
void z_sha256_update (z_sha256_t *self, const void *data, size_t size);
void z_sha256_final  (z_sha256_t *self, uint8_t digest[32]);

void z_rand_sha256   (uint64_t *seed, uint8_t digest[32]);

__Z_END_DECLS__

#endif /* _Z_SHA256_H_ */
