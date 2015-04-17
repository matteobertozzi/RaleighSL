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

#include <zcl/utest.h>

#include <zcl/math.h>

static void test_fast_u32_div (z_utest_env_t *env) {
  uint32_t i, r;
  for (i = 0; i < 0xffffffff; ++i) {
    r = z_fast_u32_div10(i);
    z_assert_u32_equals(env, i / 10, r);

    r = z_fast_u32_div100(i);
    z_assert_u32_equals(env, i / 100, r);

    r = z_fast_u32_div1000(i);
    z_assert_u32_equals(env, i / 1000, r);

    z_utest_check_timeout_interrupt(env);
  }
}

static void test_cycle (z_utest_env_t *env) {
  uint32_t seq_size;
  uint64_t seed64;
  uint32_t seed32;
  uint32_t prime;
  int i;

  seed32 = 0;
  seed64 = 0;
  seq_size = 1 << 20;
  prime = z_cycle32_prime(seq_size);
  for (i = 0; i < seq_size; ++i) {
    uint64_t v64;
    uint32_t v32;

    v32 = z_cycle32(&seed32, prime, seq_size);
    z_assert_u32_ge(env, v32, 0);
    z_assert_u32_le(env, v32, seq_size);

    v64 = z_cycle64(&seed64, prime, seq_size);
    z_assert_u64_ge(env, v64, (uint64_t)0u);
    z_assert_u64_le(env, v64, (uint64_t)seq_size);

    z_assert_u64_equals(env, (uint64_t)0u, v64 - v32);

    z_utest_check_timeout_interrupt(env);
  }
}

int main (int argc, char **argv) {
  z_utest_run(test_fast_u32_div, 10000);
  z_utest_run(test_cycle, 60000);
  return(0);
}
