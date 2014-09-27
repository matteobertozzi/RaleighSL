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

#ifndef _Z_SPOOKY2_H_
#define _Z_SPOOKY2_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/debug.h>

typedef struct z_spooky2 {
  uint64_t data[24];
  uint64_t state[12];
  uint64_t length;
  uint8_t  rem;
} z_spooky2_t;

void z_spooky2_init     (z_spooky2_t *self, uint64_t seed1, uint64_t seed2);
void z_spooky2_update   (z_spooky2_t *self, const void *data, size_t size);
void z_spooky2_final    (z_spooky2_t *self, uint64_t digest[2]);

uint32_t z_hash32_spooky2  (uint64_t seed, const void *data, size_t size);
uint64_t z_hash64_spooky2  (uint64_t seed, const void *data, size_t size);
void     z_hash128_spooky2 (const void *data, size_t size, uint64_t digest[2]);

void     z_rand_spooky2    (uint64_t *seed, uint64_t digest[2]);

__Z_END_DECLS__

#endif /* _Z_SPOOKY2_H_ */
