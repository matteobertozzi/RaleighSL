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

#include <zcl/memutil.h>
#include <zcl/utest.h>
#include <zcl/rand.h>

static void __rand_non_zero_bytes (uint8_t *buf, int length) {
  while (length > 0) {
    *buf++ = 1 + (length % 254);
    length--;
  }
}

static void test_memshared (z_utest_env_t *env) {
  uint8_t a[128];
  uint8_t b[128];
  int i;

  for (i = 0; i < 128; ++i) {
    uint64_t n;

    __rand_non_zero_bytes(a, 128);
    z_memzero(b, 128);
    z_memcpy(b, a, 128 - i);

    n = z_memshared(a, 128, b, 128);
    z_assert_u64_equals(env, 128 - i, n);
  }
}

static void test_memrshared (z_utest_env_t *env) {
  uint8_t a[128];
  uint8_t b[128];
  int i;

  for (i = 0; i < 128; ++i) {
    uint64_t n;

    __rand_non_zero_bytes(a, 128);
    z_memcpy(b + (128 - i), a + (128 - i), i);
    z_memzero(b, 128 - i);

    n = z_memrshared(a, 128, b, 128);
    z_assert_u64_equals(env, i, n);
  }
}

int main (int argc, char **argv) {
  int r = 0;
  r |= z_utest_run(test_memshared, 60000);
  r |= z_utest_run(test_memrshared, 60000);
  return(r);
}
