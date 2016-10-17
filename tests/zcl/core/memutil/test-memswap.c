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

static void test_memswap (z_utest_env_t *env) {
  uint8_t tmp[128];
  uint8_t a[128];
  uint8_t b[128];
  uint64_t seed;
  int i;

  for (i = 0; i < 128; ++i) {
    z_memzero(a, 128);
    z_memzero(b, 128);

    seed = 1;
    z_rand_bytes(&seed, a, i);
    z_rand_bytes(&seed, b, i);

    z_memswap(a, b, i);

    seed = 1;
    z_rand_bytes(&seed, tmp, i);
    z_assert_mem_equals(env, tmp, i, b, i);

    z_rand_bytes(&seed, tmp, i);
    z_assert_mem_equals(env, tmp, i, a, i);
  }
}

static void test_memswap16 (z_utest_env_t *env) {
  uint16_t a[16];
  uint16_t b[16];
  uint64_t seed;
  int i, j;

  for (i = 0; i < 16; ++i) {
    z_memzero(a, 16 * sizeof(uint16_t));
    z_memzero(b, 16 * sizeof(uint16_t));

    seed = 1;
    for (j = 0; j <= i; ++j) {
      a[j] = z_rand16(&seed);
      b[j] = z_rand16(&seed);
    }

    z_memswap16(a, b, j * sizeof(uint16_t));

    seed = 1;
    for (j = 0; j <= i; ++j) {
      z_assert_u16_equals(env, z_rand16(&seed), b[j]);
      z_assert_u16_equals(env, z_rand16(&seed), a[j]);
    }
  }
}

static void test_memswap32 (z_utest_env_t *env) {
  uint32_t a[16];
  uint32_t b[16];
  uint64_t seed;
  int i, j;

  for (i = 0; i < 16; ++i) {
    z_memzero(a, 16 * sizeof(uint32_t));
    z_memzero(b, 16 * sizeof(uint32_t));

    seed = 1;
    for (j = 0; j <= i; ++j) {
      a[j] = z_rand32(&seed);
      b[j] = z_rand32(&seed);
    }

    z_memswap32(a, b, j * sizeof(uint32_t));

    seed = 1;
    for (j = 0; j <= i; ++j) {
      z_assert_u32_equals(env, z_rand32(&seed), b[j]);
      z_assert_u32_equals(env, z_rand32(&seed), a[j]);
    }
  }
}

static void test_memswap64 (z_utest_env_t *env) {
  uint64_t a[16];
  uint64_t b[16];
  uint64_t seed;
  int i, j;

  for (i = 0; i < 16; ++i) {
    z_memzero(a, 16 * sizeof(uint64_t));
    z_memzero(b, 16 * sizeof(uint64_t));

    seed = 1;
    for (j = 0; j <= i; ++j) {
      a[j] = z_rand64(&seed);
      b[j] = z_rand64(&seed);
    }

    z_memswap64(a, b, j * sizeof(uint64_t));

    seed = 1;
    for (j = 0; j <= i; ++j) {
      z_assert_u64_equals(env, z_rand64(&seed), b[j]);
      z_assert_u64_equals(env, z_rand64(&seed), a[j]);
    }
  }
}

int main (int argc, char **argv) {
  int r = 0;
  r |= z_utest_run(test_memswap, 60000);
  r |= z_utest_run(test_memswap16, 60000);
  r |= z_utest_run(test_memswap32, 60000);
  r |= z_utest_run(test_memswap64, 60000);
  return(r);
}
