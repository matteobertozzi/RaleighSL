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
#include <zcl/utest-data.h>

#include <zcl/dblock.h>

#define BLOCK       (1 << 20)

static void __test_dblock_sset_perf_add (z_utest_env_t *env, uint8_t *block,
                                         const z_dblock_vtable_t *vtable,
                                         uint32_t klength, uint32_t vlength)
{
  z_utest_key_iter_t kiter;
  z_utest_key_iter_t viter;
  z_dblock_opts_t opts;
  uint8_t buffer[128];
  z_dblock_kv_t kv;
  int max_overhead;
  int has_space;

  z_dblock_kv_init(&kv, buffer, buffer + klength, klength, vlength);
  z_utest_key_iter_init(&kiter, Z_UTEST_KEY_ISEQ, buffer, klength, 0);
  z_utest_key_iter_init(&viter, Z_UTEST_KEY_ISEQ, buffer + klength, vlength, 0);

  has_space = 1;
  opts.blk_size = BLOCK;
  vtable->init(block, &opts);
  max_overhead = vtable->max_overhead(block);

  env->perf_bytes = 0;
  env->perf_ops   = 0;
  z_timer_start(&(env->perf_timer));
  while (has_space) {
    uint32_t avail;
    z_utest_key_iter_next(&kiter);
    z_utest_key_iter_next(&viter);
    avail = vtable->append(block, &kv);
    has_space = (avail >= (max_overhead + klength + vlength));
    env->perf_bytes += (klength + vlength);
    env->perf_ops += 1;
  }
  while (vtable->has_space(block, &kv)) {
    z_utest_key_iter_next(&kiter);
    z_utest_key_iter_next(&viter);
    vtable->append(block, &kv);
    env->perf_bytes += (klength + vlength);
    env->perf_ops += 1;
  }
  z_timer_stop(&(env->perf_timer));
}

static void __test_dblock_sset_perf_insert (z_utest_env_t *env,
                                            const z_dblock_vtable_t *vtable,
                                            uint32_t klength, uint32_t vlength)
{
  uint8_t *block;

  block = (uint8_t *)malloc(BLOCK);
  z_assert_not_null(env, block);

  __test_dblock_sset_perf_add(env, block, vtable, klength, vlength);

  free(block);
}

static void __test_dblock_sset_perf_lookup (z_utest_env_t *env,
                                            const z_dblock_vtable_t *vtable,
                                            uint32_t klength, uint32_t vlength)
{
  z_utest_key_iter_t kiter;
  z_utest_key_iter_t viter;
  uint8_t buffer[128];
  int i, j, kv_count;
  z_dblock_kv_t kv;
  uint8_t *block;

  block = (uint8_t *)malloc(BLOCK);
  z_assert_not_null(env, block);

  /* insert data */
  __test_dblock_sset_perf_add(env, block, vtable, klength, vlength);
  kv_count = env->perf_ops;

  /* test lookups */
  env->perf_bytes = 0;
  env->perf_ops   = 0;
  z_timer_start(&(env->perf_timer));
  for (j = 0; j < 2; ++j) {
    z_utest_key_iter_init(&kiter, Z_UTEST_KEY_ISEQ, buffer, klength, 0);
    z_utest_key_iter_init(&viter, Z_UTEST_KEY_ISEQ, buffer + klength, vlength, 0);
    for (i = 0; i < kv_count; ++i) {
      z_dblock_kv_init(&kv, buffer, buffer + klength, klength, vlength);
      z_utest_key_iter_next(&kiter);
      z_utest_key_iter_next(&viter);

      z_assert_true(env, vtable->lookup(block, &kv));
      env->perf_ops += 1;
    }
    for (i = 0; i < (kv_count / 2); ++i) {
      z_dblock_kv_init(&kv, buffer, buffer + klength, klength, vlength);
      z_utest_key_iter_next(&kiter);
      z_assert_false(env, vtable->lookup(block, &kv));
      env->perf_ops += 1;
    }
  }
  z_timer_stop(&(env->perf_timer));

  free(block);
}

static void __test_dblock_sset_perf_seek (z_utest_env_t *env,
                                          const z_dblock_vtable_t *vtable,
                                          uint32_t klength, uint32_t vlength)
{
  z_utest_key_iter_t kiter;
  z_utest_key_iter_t viter;
  uint8_t buffer[128];
  z_dblock_kv_t kv;
  uint8_t *block;
  int i;

  block = (uint8_t *)malloc(BLOCK);
  z_assert_not_null(env, block);

  /* insert data */
  __test_dblock_sset_perf_add(env, block, vtable, klength, vlength);

  env->perf_bytes = 0;
  env->perf_ops   = 0;
  z_timer_start(&(env->perf_timer));
  for (i = 1; i <= 64; ++i) {
    z_dblock_iter_t iter;

    /* test lookups */
    z_utest_key_iter_init(&kiter, Z_UTEST_KEY_ISEQ, buffer, klength, 0);
    z_utest_key_iter_init(&viter, Z_UTEST_KEY_ISEQ, buffer + klength, vlength, 0);

    vtable->seek(&iter, block, Z_DBLOCK_SEEK_BEGIN, NULL);
    do {
      z_dblock_kv_t ikv;

      z_dblock_kv_init(&kv, buffer, buffer + klength, klength, vlength);
      z_utest_key_iter_next(&kiter);
      z_utest_key_iter_next(&viter);

      vtable->seek_item(&iter, &ikv);
      env->perf_ops += 1;
    } while (vtable->seek_next(&iter));
  }
  z_timer_stop(&(env->perf_timer));

  free(block);
}

/* ============================================================================
 */
static void test_dblock_avl16_perf_append (z_utest_env_t *env) {
  __test_dblock_sset_perf_insert(env, &z_dblock_avl16_map, 4, 8);
}

static void test_dblock_avl16_perf_lookup (z_utest_env_t *env) {
  __test_dblock_sset_perf_lookup(env, &z_dblock_avl16_map, 4, 8);
}

static void test_dblock_avl16_perf_seek (z_utest_env_t *env) {
  __test_dblock_sset_perf_seek(env, &z_dblock_avl16_map, 4, 8);
}

/* ============================================================================
 */
static void test_dblock_avl16e_perf_append (z_utest_env_t *env) {
  __test_dblock_sset_perf_insert(env, &z_dblock_avl16e_map, 4, 8);
}

static void test_dblock_avl16e_perf_lookup (z_utest_env_t *env) {
  __test_dblock_sset_perf_lookup(env, &z_dblock_avl16e_map, 4, 8);
}

static void test_dblock_avl16e_perf_seek (z_utest_env_t *env) {
  __test_dblock_sset_perf_seek(env, &z_dblock_avl16e_map, 4, 8);
}

/* ============================================================================
 */
static void test_dblock_log_perf_append (z_utest_env_t *env) {
  __test_dblock_sset_perf_insert(env, &z_dblock_log_map, 4, 8);
}

static void test_dblock_log_perf_lookup (z_utest_env_t *env) {
  __test_dblock_sset_perf_lookup(env, &z_dblock_log_map, 32, 64);
}

static void test_dblock_log_perf_seek (z_utest_env_t *env) {
  __test_dblock_sset_perf_seek(env, &z_dblock_log_map, 4, 8);
}

int main (int argc, char **argv) {
  z_utest_run(test_dblock_avl16_perf_append, 0);
  z_utest_run(test_dblock_avl16_perf_lookup, 0);
  z_utest_run(test_dblock_avl16_perf_seek, 0);

  z_utest_run(test_dblock_avl16e_perf_append, 0);
  z_utest_run(test_dblock_avl16e_perf_lookup, 0);
  z_utest_run(test_dblock_avl16e_perf_seek, 0);

  z_utest_run(test_dblock_log_perf_append, 0);
  z_utest_run(test_dblock_log_perf_lookup, 0);
  z_utest_run(test_dblock_log_perf_seek, 0);
  return(0);
}
