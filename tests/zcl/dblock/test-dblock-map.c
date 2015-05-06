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

#include <zcl/endian.h>
#include <zcl/utest.h>
#include <zcl/math.h>

#include <zcl/humans.h>
#include <zcl/dblock.h>

#define BLOCK       (96 << 10)
#define MIN_KVS     ((uint64_t)450u)

static void __build_keybuf (uint8_t *buf, uint64_t key, uint32_t size) {
  z_put_uint64_be(key, buf, 0);
  memset(buf + 8, key & 0xff, size - 8);
}

static void __build_valbuf (uint8_t *buf, uint32_t value, uint32_t size) {
  if (size > 0) {
    memcpy(buf, &value, 4);
    memset(buf + 4, value & 0xff, size - 4);
  }
}

static uint64_t __test_dblock_append (z_utest_env_t *env,
                                          const z_dblock_vtable_t *vtable,
                                          uint8_t *block,
                                          const int ksize, int vsize)
{
  uint8_t kbuf[128], vbuf[128];
  z_dblock_opts_t opts;
  uint64_t kv_count = 0;
  int has_space = 1;
  int max_overhead;
  z_dblock_kv_t kv;
  uint32_t value;
  uint64_t key;

  z_dblock_kv_init(&kv, kbuf, vbuf, ksize, vsize);

  opts.blk_size = BLOCK;
  vtable->init(block, &opts);
  max_overhead = vtable->max_overhead(block);

  key = 0;
  value = 0xffffffff;
  while (has_space) {
    uint32_t avail;
    __build_keybuf(kbuf, key, ksize);
    __build_valbuf(vbuf, value, vsize);

    avail = vtable->append(block, &kv);
    has_space = (avail >= (max_overhead + ksize + vsize));
    kv_count++;
    value--;
    key++;
  }

  while (vtable->has_space(block, &kv)) {
    __build_keybuf(kbuf, key, ksize);
    __build_valbuf(vbuf, value, vsize);

    vtable->append(block, &kv);
    kv_count++;
    value--;
    key++;
  }

  return(kv_count);
}

static void __test_dblock_insert (z_utest_env_t *env,
                                      const z_dblock_vtable_t *vtable,
                                      uint8_t *block, uint64_t kv_count,
                                      const int ksize, int vsize)
{
  uint8_t kbuf[128], vbuf[128];
  z_dblock_opts_t opts;
  z_dblock_kv_t kv;
  uint32_t prime;
  uint32_t value;
  uint32_t seed;
  uint64_t key;
  uint32_t i;

  z_dblock_kv_init(&kv, kbuf, vbuf, ksize, vsize);

  opts.blk_size = BLOCK;
  vtable->init(block, &opts);

  seed = 0;
  prime = z_cycle32_prime(kv_count);
  for (i = 0; i < kv_count; ++i) {
    key = z_cycle32(&seed, prime, kv_count);
    value = 0xffffffff - key;
    __build_keybuf(kbuf, key, ksize);
    __build_valbuf(vbuf, value, vsize);

    vtable->insert(block, &kv);
  }
}

static void __test_dblock_prepend (z_utest_env_t *env,
                                       const z_dblock_vtable_t *vtable,
                                       uint8_t *block, uint64_t kv_count,
                                       const int ksize, int vsize)
{
  uint8_t kbuf[128], vbuf[128];
  z_dblock_opts_t opts;
  z_dblock_kv_t kv;
  uint32_t value;
  uint64_t key;

  z_dblock_kv_init(&kv, kbuf, vbuf, ksize, vsize);

  opts.blk_size = BLOCK;
  vtable->init(block, &opts);

  key = kv_count;
  value = 0xffffffff - kv_count;
  while (kv_count > 0) {
    key--;
    value++;
    __build_keybuf(kbuf, key, ksize);
    __build_valbuf(vbuf, value, vsize);

    vtable->prepend(block, &kv);
    kv_count--;
  }

  z_assert_u64_equals(env, (uint64_t)0u, key);
  z_assert_u64_equals(env, (uint64_t)0u, kv_count);
}

static void __test_dblock_lookup (z_utest_env_t *env,
                                      const z_dblock_vtable_t *vtable,
                                      uint8_t *block, uint64_t kv_count,
                                      const int ksize, int vsize)
{
  uint8_t kbuf[128], vbuf[128];
  uint32_t value;
  uint64_t key;
  int i;

  key = 0;
  value = 0xffffffff;
  for (i = 0; i < kv_count; ++i) {
    z_dblock_kv_t kv;
    __build_keybuf(kbuf, key, ksize);
    __build_valbuf(vbuf, value, vsize);
    z_dblock_kv_init(&kv, kbuf, NULL, ksize, 0);

    z_assert_true(env, vtable->lookup(block, &kv));
    z_assert_mem_equals(env, kbuf, ksize, kv.key, kv.klength);
    z_assert_mem_equals(env, vbuf, vsize, kv.value, kv.vlength);

    value--;
    key++;
  }

  for (i = 0; i < (kv_count / 2); ++i) {
    z_dblock_kv_t kv;
    __build_keybuf(kbuf, key, ksize);
    z_dblock_kv_init(&kv, kbuf, NULL, ksize, 0);

    z_assert_false(env, vtable->lookup(block, &kv));
    key++;
  }
}

static void __test_dblock_seek_to (z_utest_env_t *env,
                                       const z_dblock_vtable_t *vtable,
                                       uint8_t *block, const int ksize, int vsize,
                                       z_dblock_seek_t seek_pos,
                                       uint64_t seek_key, uint64_t expected_key,
                                       int has_prev, int has_next)
{
  uint8_t kbuf[128], vbuf[128];
  z_dblock_iter_t iter;
  z_dblock_kv_t kv;
  int has_data;

  __build_keybuf(kbuf, seek_key, ksize);
  __build_valbuf(vbuf, 0xffffffff - seek_key, vsize);
  z_dblock_kv_init(&kv, kbuf, NULL, ksize, 0);
  has_data = vtable->seek(&iter, block, seek_pos, &kv);
  z_assert_true(env, has_data);

  vtable->seek_item(&iter, &kv);
  __build_keybuf(kbuf, expected_key, ksize);
  __build_valbuf(vbuf, 0xffffffff - expected_key, vsize);
  z_assert_mem_equals(env, kbuf, ksize, kv.key, kv.klength);
  z_assert_mem_equals(env, vbuf, vsize, kv.value, kv.vlength);

  has_data = vtable->seek_next(&iter);
  if (has_next) {
    z_assert_true(env, has_data);
    vtable->seek_item(&iter, &kv);
    __build_keybuf(kbuf, expected_key + 1, ksize);
    __build_valbuf(vbuf, 0xffffffff - expected_key - 1, vsize);
    z_assert_mem_equals(env, kbuf, ksize, kv.key, kv.klength);
    z_assert_mem_equals(env, vbuf, vsize, kv.value, kv.vlength);
  } else {
    z_assert_false(env, has_data);
  }

  __build_keybuf(kbuf, seek_key, ksize);
  __build_valbuf(vbuf, 0xffffffff - seek_key, vsize);
  z_dblock_kv_init(&kv, kbuf, NULL, ksize, 0);
  has_data = vtable->seek(&iter, block, seek_pos, &kv);
  z_assert_true(env, has_data);

  has_data = vtable->seek_prev(&iter);
  if (has_prev) {
    z_assert_true(env, has_data);
    vtable->seek_item(&iter, &kv);
    __build_keybuf(kbuf, expected_key - 1, ksize);
    __build_valbuf(vbuf, 0xffffffff - expected_key + 1, vsize);
    z_assert_mem_equals(env, kbuf, ksize, kv.key, kv.klength);
    z_assert_mem_equals(env, vbuf, vsize, kv.value, kv.vlength);
  } else {
    z_assert_false(env, has_data);
  }
}

static void __test_dblock_seek (z_utest_env_t *env,
                                    const z_dblock_vtable_t *vtable,
                                    uint8_t *block, uint64_t kv_count,
                                     const int ksize, int vsize)
{
  /* seek found */
  __test_dblock_seek_to(env, vtable, block, ksize, vsize,
                            Z_DBLOCK_SEEK_EQ, 50, 50, 1, 1);
  z_utest_check_failure(env);

  __test_dblock_seek_to(env, vtable, block, ksize, vsize,
                            Z_DBLOCK_SEEK_EQ, 0, 0, 0, 1);
  z_utest_check_failure(env);

  __test_dblock_seek_to(env, vtable, block, ksize, vsize,
                            Z_DBLOCK_SEEK_LT, 50, 49, 1, 1);
  z_utest_check_failure(env);

  __test_dblock_seek_to(env, vtable, block, ksize, vsize,
                            Z_DBLOCK_SEEK_LT, 0xffffffffffff, kv_count - 1, 1, 0);
  z_utest_check_failure(env);

  __test_dblock_seek_to(env, vtable, block, ksize, vsize,
                            Z_DBLOCK_SEEK_LE, 50, 50, 1, 1);
  z_utest_check_failure(env);

  __test_dblock_seek_to(env, vtable, block, ksize, vsize,
                            Z_DBLOCK_SEEK_LE, 0, 0, 0, 1);
  z_utest_check_failure(env);

  __test_dblock_seek_to(env, vtable, block, ksize, vsize,
                            Z_DBLOCK_SEEK_LE, 0xffffffffffff, kv_count - 1, 1, 0);
  z_utest_check_failure(env);

  __test_dblock_seek_to(env, vtable, block, ksize, vsize,
                            Z_DBLOCK_SEEK_GT, 50, 51, 1, 1);
  z_utest_check_failure(env);

  __test_dblock_seek_to(env, vtable, block, ksize, vsize,
                            Z_DBLOCK_SEEK_GT, 0, 1, 1, 1);
  z_utest_check_failure(env);

  __test_dblock_seek_to(env, vtable, block, ksize, vsize,
                            Z_DBLOCK_SEEK_GE, 50, 50, 1, 1);
  z_utest_check_failure(env);

  __test_dblock_seek_to(env, vtable, block, ksize, vsize,
                            Z_DBLOCK_SEEK_GE, 0, 0, 0, 1);
  z_utest_check_failure(env);

  /* seek not found */
  do {
    z_dblock_iter_t iter;
    uint8_t kbuf[128];
    z_dblock_kv_t kv;

    __build_keybuf(kbuf, 0, ksize);
    z_dblock_kv_init(&kv, kbuf, NULL, ksize, 0);

    z_assert_false(env, vtable->seek(&iter, block, Z_DBLOCK_SEEK_LT, &kv));
    z_utest_check_failure(env);

    __build_keybuf(kbuf, 0xffffffffffff, ksize);
    z_dblock_kv_init(&kv, kbuf, NULL, ksize, 0);

    z_assert_false(env, vtable->seek(&iter, block, Z_DBLOCK_SEEK_EQ, &kv));
    z_utest_check_failure(env);

    z_assert_false(env, vtable->seek(&iter, block, Z_DBLOCK_SEEK_GT, &kv));
    z_utest_check_failure(env);

    z_assert_false(env, vtable->seek(&iter, block, Z_DBLOCK_SEEK_GE, &kv));
    z_utest_check_failure(env);
  } while (0);
}

static void __test_dblock_seek_next (z_utest_env_t *env,
                                         const z_dblock_vtable_t *vtable,
                                         uint8_t *block, uint64_t kv_count,
                                         const int ksize, int vsize)
{
  uint8_t kbuf[128], vbuf[128];
  z_dblock_iter_t iter;
  uint32_t value;
  uint64_t key;

  key = 0;
  value = 0xffffffff;
  vtable->seek(&iter, block, Z_DBLOCK_SEEK_BEGIN, NULL);
  do {
    z_dblock_kv_t kv;
    __build_keybuf(kbuf, key, ksize);
    __build_valbuf(vbuf, value, vsize);
    z_dblock_kv_init(&kv, NULL, NULL, 0, 0);

    vtable->seek_item(&iter, &kv);
    z_assert_mem_equals(env, kbuf, ksize, kv.key, kv.klength);
    z_assert_mem_equals(env, vbuf, vsize, kv.value, kv.vlength);

    value--;
    key++;
  } while (vtable->seek_next(&iter));

  z_assert_u64_equals(env, kv_count, key);
}

static void __test_dblock_seek_prev (z_utest_env_t *env,
                                         const z_dblock_vtable_t *vtable,
                                         uint8_t *block, uint64_t kv_count,
                                         const int ksize, int vsize)
{
  uint8_t kbuf[128], vbuf[128];
  z_dblock_iter_t iter;
  uint32_t value;
  uint64_t key;

  key = kv_count;
  value = 0xffffffff - kv_count;
  vtable->seek(&iter, block, Z_DBLOCK_SEEK_END, NULL);
  do {
    z_dblock_kv_t kv;

    key--;
    value++;
    __build_keybuf(kbuf, key, ksize);
    __build_valbuf(vbuf, value, vsize);
    z_dblock_kv_init(&kv, NULL, NULL, 0, 0);

    vtable->seek_item(&iter, &kv);
    z_assert_mem_equals(env, kbuf, ksize, kv.key, kv.klength);
    z_assert_mem_equals(env, vbuf, vsize, kv.value, kv.vlength);
  } while (vtable->seek_prev(&iter));

  z_assert_u32_equals(env, 0xffffffff, value);
  z_assert_u64_equals(env, (uint64_t)0u, key);
}

static void __test_dblock_edges (z_utest_env_t *env,
                                     const z_dblock_vtable_t *vtable,
                                     uint8_t *block, uint64_t kv_count,
                                     const int ksize, int vsize)
{
  uint8_t kbuf[128], vbuf[128];
  z_dblock_kv_t kv;

  __build_keybuf(kbuf, 0, ksize);
  __build_valbuf(vbuf, 0xffffffff, vsize);
  vtable->first_key(block, &kv);
  z_assert_mem_equals(env, kbuf, ksize, kv.key, kv.klength);
  z_assert_mem_equals(env, vbuf, vsize, kv.value, kv.vlength);

  __build_keybuf(kbuf, kv_count - 1, ksize);
  __build_valbuf(vbuf, 0xffffffff - kv_count + 1, vsize);
  vtable->last_key(block, &kv);
  z_assert_mem_equals(env, kbuf, ksize, kv.key, kv.klength);
  z_assert_mem_equals(env, vbuf, vsize, kv.value, kv.vlength);
}

static void __test_dblock_read (z_utest_env_t *env,
                                const z_dblock_vtable_t *vtable,
                                uint8_t *block, uint64_t kv_count,
                                const int ksize, int vsize)
{
  /* test lookups */
  __test_dblock_lookup(env, vtable, block, kv_count, ksize, vsize);
  z_utest_check_failure(env);

  /* test seek */
  __test_dblock_seek_next(env, vtable, block, kv_count, ksize, vsize);
  z_utest_check_failure(env);
  __test_dblock_seek_prev(env, vtable, block, kv_count, ksize, vsize);
  z_utest_check_failure(env);

  /* test seek-to */
  __test_dblock_seek(env, vtable, block, kv_count, ksize, vsize);
  z_utest_check_failure(env);

  /* test edges */
  __test_dblock_edges(env, vtable, block, kv_count, ksize, vsize);
  z_utest_check_failure(env);
}

static void __test_dblock_sort (z_utest_env_t *env,
                                const z_dblock_vtable_t *vtable,
                                uint8_t *block, uint64_t kv_count)
{
  z_dblock_kv_t a_kv, b_kv;
  uint32_t *index;
  uint32_t i;

  index = (uint32_t *) malloc(kv_count * sizeof(uint32_t));
  z_assert_not_null(env, index);

  z_dblock_build_index32(vtable, block, index);
  for (i = 1; i < kv_count; ++i) {
    vtable->get_iptr(block, index[i - 1], &a_kv);
    vtable->get_iptr(block, index[i + 0], &b_kv);
    z_assert_mem_lt(env, a_kv.key, a_kv.klength, b_kv.key, b_kv.klength);
  }

  free(index);
}

static void __test_dblock_smap (z_utest_env_t *env,
                                const z_dblock_vtable_t *vtable,
                                const int ksize, int vsize)
{
  uint64_t kv_count;
  uint8_t *block;

  block = (uint8_t *)malloc(BLOCK);
  z_assert_not_null(env, block);

  /* append data */
  kv_count = __test_dblock_append(env, vtable, block, ksize, vsize);
  z_utest_check_failure(env);
  z_assert_u64_gt(env, kv_count, MIN_KVS);
  __test_dblock_read(env, vtable, block, kv_count, ksize, vsize);
  z_utest_check_failure(env);
  __test_dblock_sort(env, vtable, block, kv_count);
  z_utest_check_failure(env);

  /* insert data */
  __test_dblock_insert(env, vtable, block, kv_count, ksize, vsize);
  z_utest_check_failure(env);
  __test_dblock_read(env, vtable, block, kv_count, ksize, vsize);
  z_utest_check_failure(env);
  __test_dblock_sort(env, vtable, block, kv_count);
  z_utest_check_failure(env);

  /* prepend data */
  __test_dblock_prepend(env, vtable, block, kv_count, ksize, vsize);
  z_utest_check_failure(env);
  __test_dblock_read(env, vtable, block, kv_count, ksize, vsize);
  z_utest_check_failure(env);
  __test_dblock_sort(env, vtable, block, kv_count);
  z_utest_check_failure(env);

  free(block);
}

static void __test_dblock_map (z_utest_env_t *env,
                               const z_dblock_vtable_t *vtable,
                               const int ksize, int vsize)
{
  uint64_t kv_count;
  uint8_t *block;

  block = (uint8_t *)malloc(BLOCK);
  z_assert_not_null(env, block);

  /* append data */
  kv_count = __test_dblock_append(env, vtable, block, ksize, vsize);
  z_utest_check_failure(env);
  z_assert_u64_gt(env, kv_count, MIN_KVS);
  __test_dblock_read(env, vtable, block, kv_count, ksize, vsize);
  z_utest_check_failure(env);
  __test_dblock_sort(env, vtable, block, kv_count);
  z_utest_check_failure(env);

  /* insert data */
  __test_dblock_insert(env, vtable, block, kv_count, ksize, vsize);
  z_utest_check_failure(env);
  __test_dblock_lookup(env, vtable, block, kv_count, ksize, vsize);
  z_utest_check_failure(env);
  __test_dblock_edges(env, vtable, block, kv_count, ksize, vsize);
  z_utest_check_failure(env);
  __test_dblock_sort(env, vtable, block, kv_count);
  z_utest_check_failure(env);

  /* prepend data */
  __test_dblock_prepend(env, vtable, block, kv_count, ksize, vsize);
  z_utest_check_failure(env);
  __test_dblock_lookup(env, vtable, block, kv_count, ksize, vsize);
  z_utest_check_failure(env);
  __test_dblock_edges(env, vtable, block, kv_count, ksize, vsize);
  z_utest_check_failure(env);
  __test_dblock_sort(env, vtable, block, kv_count);
  z_utest_check_failure(env);

  free(block);
}

/* ============================================================================
 */
static void test_dblock_avl16_k8v4 (z_utest_env_t *env) {
  __test_dblock_smap(env, &z_dblock_avl16_map, 8, 4);
}

static void test_dblock_avl16_k15v15 (z_utest_env_t *env) {
  __test_dblock_smap(env, &z_dblock_avl16_map, 15, 15);
}

static void test_dblock_avl16_k32v0 (z_utest_env_t *env) {
  __test_dblock_smap(env, &z_dblock_avl16_map, 32, 0);
}

static void test_dblock_avl16_k64v128 (z_utest_env_t *env) {
  __test_dblock_smap(env, &z_dblock_avl16_map, 64, 128);
}

/* ============================================================================
 */
static void test_dblock_avl16e_k8v4 (z_utest_env_t *env) {
  __test_dblock_smap(env, &z_dblock_avl16e_map, 8, 4);
}

static void test_dblock_avl16e_k15v15 (z_utest_env_t *env) {
  __test_dblock_smap(env, &z_dblock_avl16e_map, 15, 15);
}

static void test_dblock_avl16e_k32v0 (z_utest_env_t *env) {
  __test_dblock_smap(env, &z_dblock_avl16e_map, 32, 0);
}

static void test_dblock_avl16e_k64v128 (z_utest_env_t *env) {
  __test_dblock_smap(env, &z_dblock_avl16e_map, 64, 128);
}

/* ============================================================================
 */
static void test_dblock_log_k8v4 (z_utest_env_t *env) {
  __test_dblock_map(env, &z_dblock_log_map, 8, 4);
}

static void test_dblock_log_k15v15 (z_utest_env_t *env) {
  __test_dblock_map(env, &z_dblock_log_map, 15, 15);
}

static void test_dblock_log_k32v0 (z_utest_env_t *env) {
  __test_dblock_map(env, &z_dblock_log_map, 32, 0);
}

static void test_dblock_log_k64v128 (z_utest_env_t *env) {
  __test_dblock_map(env, &z_dblock_log_map, 64, 128);
}

static z_dblock_overlap_t __test_dblock_overlap (uint64_t a_first_key,
                                                     uint64_t a_last_key,
                                                     uint64_t b_first_key,
                                                     uint64_t b_last_key)
{
  z_dblock_opts_t opts;
  uint8_t a_block[128];
  uint8_t b_block[128];
  z_dblock_kv_t kv;
  uint8_t buf[64];

  kv.key = buf;
  kv.value = NULL;
  kv.klength = 8;
  kv.vlength = 0;

  opts.blk_size = sizeof(a_block);
  z_dblock_log_map.init(a_block, &opts);
  __build_keybuf(buf, a_first_key, kv.klength);
  z_dblock_log_map.append(a_block, &kv);
  __build_keybuf(buf, a_last_key, kv.klength);
  z_dblock_log_map.append(a_block, &kv);

  opts.blk_size = sizeof(b_block);
  z_dblock_avl16_map.init(b_block, &opts);
  __build_keybuf(buf, b_first_key, kv.klength);
  z_dblock_avl16_map.append(b_block, &kv);
  __build_keybuf(buf, b_last_key, kv.klength);
  z_dblock_avl16_map.append(b_block, &kv);

  return(z_dblock_overlap(&z_dblock_log_map, a_block,
                              &z_dblock_avl16_map, b_block));
}

static void test_dblock_overlap (z_utest_env_t *env) {
  /* |--- X ---|  |--- Y ---| */
  z_assert_u32_equals(env, Z_DBLOCK_OVERLAP_NO_LEFT,
                      __test_dblock_overlap(10, 20, 30, 40));
  z_assert_u32_equals(env, Z_DBLOCK_OVERLAP_NO_RIGHT,
                      __test_dblock_overlap(30, 40, 10, 20));

  /* |--- X ---|--- Y ---| */
  z_assert_u32_equals(env, Z_DBLOCK_OVERLAP_YES_LEFT,
                      __test_dblock_overlap(10, 20, 20, 40));
  z_assert_u32_equals(env, Z_DBLOCK_OVERLAP_YES_RIGHT,
                      __test_dblock_overlap(20, 40, 10, 20));

  /*
   * |----- X -----|
   *   |--- Y ---|
   */
  z_assert_u32_equals(env, Z_DBLOCK_OVERLAP_YES,
                      __test_dblock_overlap(10, 100, 20, 80));
  z_assert_u32_equals(env, Z_DBLOCK_OVERLAP_YES,
                      __test_dblock_overlap(20, 80, 10, 100));

  /*
   * |----- X -----|
   *           |--- Y ---|
   */
  z_assert_u32_equals(env, Z_DBLOCK_OVERLAP_YES,
                      __test_dblock_overlap(10, 20, 15, 30));
  z_assert_u32_equals(env, Z_DBLOCK_OVERLAP_YES,
                      __test_dblock_overlap(15, 30, 10, 20));
}

int main (int argc, char **argv) {
  z_utest_run(test_dblock_avl16_k8v4, 0);
  z_utest_run(test_dblock_avl16_k15v15, 0);
  z_utest_run(test_dblock_avl16_k32v0, 0);
  z_utest_run(test_dblock_avl16_k64v128, 0);

  z_utest_run(test_dblock_avl16e_k8v4, 0);
  z_utest_run(test_dblock_avl16e_k15v15, 0);
  z_utest_run(test_dblock_avl16e_k32v0, 0);
  z_utest_run(test_dblock_avl16e_k64v128, 0);

  z_utest_run(test_dblock_log_k8v4, 0);
  z_utest_run(test_dblock_log_k15v15, 0);
  z_utest_run(test_dblock_log_k32v0, 0);
  z_utest_run(test_dblock_log_k64v128, 0);

  z_utest_run(test_dblock_overlap, 0);
  return(0);
}
