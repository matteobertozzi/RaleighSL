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

#include <zcl/utest-data.h>

void z_utest_key_iter_init (z_utest_key_iter_t *iter,
                            z_utest_key_type_t type,
                            uint8_t *key, uint32_t klength,
                            uint64_t seed)
{
  iter->type    = type;
  iter->key     = key;
  iter->klength = klength;
  iter->seed    = seed;
  switch (type) {
    case Z_UTEST_KEY_RAND_UNIQ:
      memset(key, 0, klength);
      break;
    case Z_UTEST_KEY_RAND:
      memset(key, 0, klength);
      break;
    case Z_UTEST_KEY_ISEQ:
      memset(key, 0, klength);
      break;
    case Z_UTEST_KEY_DSEQ:
      memset(key, 0xff, klength);
      break;
  }
}

void z_utest_key_iter_next (z_utest_key_iter_t *iter) {
  switch (iter->type) {
    case Z_UTEST_KEY_RAND_UNIQ:
      // TODO
      break;
    case Z_UTEST_KEY_RAND:
      z_rand_bytes(&((iter)->seed), iter->key, iter->klength);
      break;
    case Z_UTEST_KEY_ISEQ: {
      uint8_t *p = iter->key + iter->klength - 1;
      const uint8_t *pe = iter->key;
      while (p >= pe && ++(*p) == 0) p--;
      break;
    }
    case Z_UTEST_KEY_DSEQ: {
      uint8_t *p = iter->key + iter->klength - 1;
      const uint8_t *pe = iter->key;
      while (p >= pe && (*p)-- == 0) p--;
      break;
    }
  }
}
