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

#ifndef _Z_UTEST_DATA_H_
#define _Z_UTEST_DATA_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/rand.h>

typedef enum z_utest_key_type {
  Z_UTEST_KEY_RAND_UNIQ,
  Z_UTEST_KEY_RAND,
  Z_UTEST_KEY_ISEQ,
  Z_UTEST_KEY_DSEQ,
} z_utest_key_type_t;

typedef struct z_utest_key_iter {
  z_utest_key_type_t type;
  uint32_t klength;
  uint8_t *key;
  uint64_t seed;
} z_utest_key_iter_t;

void z_utest_key_iter_init (z_utest_key_iter_t *iter,
                            z_utest_key_type_t type,
                            uint8_t *key, uint32_t klength,
                            uint64_t seed);
void z_utest_key_iter_next (z_utest_key_iter_t *iter);

__Z_END_DECLS__

#endif /* !_Z_UTEST_DATA_H_ */
