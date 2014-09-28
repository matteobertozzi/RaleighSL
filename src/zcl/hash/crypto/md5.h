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

#ifndef _Z_MD5_H_
#define _Z_MD5_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/debug.h>

typedef struct z_md5 {
  uint32_t total[2];
  uint32_t state[4];
  uint8_t  buffer[64];
} z_md5_t;

void z_md5_init     (z_md5_t *self);
void z_md5_update   (z_md5_t *self, const void *data, size_t size);
void z_md5_final    (z_md5_t *self, uint8_t digest[16]);

void z_hash128_md5  (const void *data, size_t size, uint8_t digest[16]);
void z_rand_md5     (uint64_t *seed, uint8_t digest[16]);

__Z_END_DECLS__

#endif /* _Z_MD5_H_ */
