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

#ifndef _Z_SHA512_H_
#define _Z_SHA512_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

typedef struct z_sha512 {
  uint64_t total[2];
  uint64_t state[8];
  uint8_t  buffer[128];
} z_sha512_t;

void z_sha512_init   (z_sha512_t *self);
void z_sha512_update (z_sha512_t *self, const void *data, size_t size);
void z_sha512_final  (z_sha512_t *self, uint8_t digest[64]);

void z_rand_sha512   (uint64_t *seed, uint8_t digest[64]);

__Z_END_DECLS__

#endif /* _Z_SHA512_H_ */
