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

#ifndef _Z_MT19937_H_
#define _Z_MT19937_H_

#include <zcl/macros.h>

typedef struct z_mt19937_32 {
  uint32_t mt[624];
  uint32_t mti;
} z_mt19937_32_t;

typedef struct z_mt19937_64 {
  uint64_t mt[312];
  uint32_t mti;
} z_mt19937_64_t;

void     z_mt19937_32_init (z_mt19937_32_t *self, uint32_t seed);
uint32_t z_mt19937_32_next (z_mt19937_32_t *self);

void     z_mt19937_64_init (z_mt19937_64_t *self, uint64_t seed);
uint64_t z_mt19937_64_next (z_mt19937_64_t *self);

#endif /* !_Z_MT19937_H_ */
