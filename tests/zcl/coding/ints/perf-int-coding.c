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

#include <zcl/int-coding.h>

static void test_uint_encode (z_utest_env_t *env) {
  uint8_t buffer[16];
  uint64_t i;

  for (i = 0; i < 0x7ffffffff; i += 67) {
    z_uint_encode(buffer, z_uint64_size(i), i);
    env->perf_ops++;
  }
}

static void test_uint_coding (z_utest_env_t *env) {
  uint8_t buffer[16];
  uint64_t i;

  for (i = 0; i < 0x5ffffffff; i += 67) {
    const uint8_t size = z_uint64_size(i);
    uint64_t x;
    z_uint64_encode(buffer, size, i);
    z_uint64_decode(buffer, size, &x);
    env->perf_ops++;
  }
}

static void test_vint_encode (z_utest_env_t *env) {
  uint8_t buffer[16];
  uint64_t i;

  for (i = 0; i < 0x7ffffffff; i += 67) {
    z_vint64_encode(buffer, i);
    env->perf_ops++;
  }
}

static void test_vint_coding (z_utest_env_t *env) {
  uint8_t buffer[16];
  uint64_t i;

  for (i = 0; i < 0x5ffffffff; i += 67) {
    uint64_t x;
    z_vint64_encode(buffer, i);
    z_vint64_decode(buffer, &x);
    env->perf_ops++;
  }
}

int main (int argc, char **argv) {
  z_utest_run(test_uint_encode, 10000);
  z_utest_run(test_uint_coding, 10000);
  z_utest_run(test_vint_encode, 10000);
  z_utest_run(test_vint_coding, 10000);
  return(0);
}
