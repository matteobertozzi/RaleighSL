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

#include <zcl/histogram.h>
#include <zcl/utest.h>

static void test_simple (z_utest_env_t *env) {
  z_histogram_t histo;
  uint64_t bounds[32];
  uint64_t events[32];
  int nbuckets, i;

  nbuckets = 32;
  for (i = 0; i < 31; ++i) {
    bounds[i] = i;
  }
  bounds[31] = 0xffffffffffffffffll;

  z_histogram_init(&histo, bounds, events, nbuckets);

  // Test Insert
  for (i = 0; i < (nbuckets + 4); ++i) {
    z_histogram_add(&histo, i);
  }

  for (i = 0; i < (nbuckets - 1); ++i) {
    z_assert_u64_equals(env, 1, histo.events[i]);
  }
  z_assert_u64_equals(env, 32 + 3, histo.max);
  z_assert_u64_equals(env, 5, histo.events[i]);

  // Test Clear
  z_histogram_clear(&histo, nbuckets);
  for (i = 0; i < (nbuckets - 1); ++i) {
    z_assert_u64_equals(env, 0, histo.events[i]);
  }
  z_assert_u64_equals(env, 0, histo.max);
  z_assert_u64_equals(env, 0, histo.events[i]);
}

static void test_merge (z_utest_env_t *env) {
  z_histogram_t histo[3];
  uint64_t bounds[16];
  uint64_t events[3 * 16];
  int nbuckets, i, j;

  nbuckets = 16;
  for (i = 0; i < 16; ++i) {
    bounds[i] = i;
  }
  bounds[15] = 0xffffffffffffffffll;

  // insert
  for (i = 0; i < 3; ++i) {
    z_histogram_init(histo + i, bounds, events + (i * nbuckets), nbuckets);
    for (j = 0; j < (nbuckets + 4); ++j) {
      z_histogram_add(histo + i, j);
    }
  }

  // test merge
  z_histogram_merge(histo + 0, 2, histo + 1, histo + 2);
  for (i = 0; i < (nbuckets - 1); ++i) {
    z_assert_u64_equals(env, 3, histo[0].events[i]);
  }
  z_assert_u64_equals(env, 16 + 3, histo[0].max);
  z_assert_u64_equals(env, 5, histo[0].events[i]);
}


int main (int argc, char **argv) {
  int r = 0;
  r |= z_utest_run(test_simple, 60000);
  r |= z_utest_run(test_merge, 60000);
  return(r);
}
