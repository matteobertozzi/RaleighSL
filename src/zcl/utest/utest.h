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

#ifndef _Z_UTEST_H_
#define _Z_UTEST_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/memutil.h>
#include <zcl/macros.h>
#include <zcl/humans.h>
#include <zcl/timer.h>
#include <zcl/debug.h>

typedef struct z_utest_env z_utest_env_t;

typedef void (*z_utest_func_t) (z_utest_env_t *env);

struct z_utest_env {
  /* failure fields */
  char stack_trace[512];
  char fail_msg[512];
  int  fail_timeout;
  int  fail_line;
  /* generic fields */
  z_timer_t timer;
  /* perf fields */
  uint64_t  perf_bytes;
  uint64_t  perf_ops;
  z_timer_t perf_timer;
};

/* ============================================================================
 *  Assert generic
 */
#define z_assert(env, cond, msg, ...)                                         \
  do {                                                                        \
    if (!(cond)) {                                                            \
      (env)->fail_line = __LINE__;                                            \
      z_stack_trace((env)->stack_trace, sizeof((env)->stack_trace), 0);       \
      snprintf((env)->fail_msg, sizeof((env)->fail_msg),                      \
               #cond ": " msg, ##__VA_ARGS__);                                \
      return;                                                                 \
    }                                                                         \
  } while (0)

/* ============================================================================
 *  Assert null pointer
 */
#define z_assert_null(env, actual)                                            \
  z_assert(env, ((actual) == NULL), "expected NULL got %p", actual)

#define z_assert_not_null(env, actual)                                        \
  z_assert(env, ((actual) != NULL), "expected not NULL")

/* ============================================================================
 *  Assert null pointer
 */
#define z_assert_true(env, actual)                                            \
  z_assert(env, !!(actual) == 1, "expected true")

#define z_assert_false(env, actual)                                           \
  z_assert(env, !!(actual) == 0, "expected false")

/* ============================================================================
 *  Assert numeric equals
 */
#define __z_assert_equals(env, frmt, expected, actual)                        \
  z_assert(env, ((expected) == (actual)),                                     \
           "expected %"frmt" got %"frmt, expected, actual)

#define __z_assert_not_equals(env, frmt, expected, actual)                    \
  z_assert(env, ((expected) != (actual)),                                     \
           "expected %"frmt" got %"frmt, expected, actual)

#define z_assert_u32_equals(env, expected, actual)                            \
  __z_assert_equals(env, PRIu32, expected, actual)

#define z_assert_u32_not_equals(env, expected, actual)                        \
  __z_assert_not_equals(env, PRIu32, expected, actual)

#define z_assert_u64_equals(env, expected, actual)                            \
  __z_assert_equals(env, PRIu64, expected, actual)

#define z_assert_u64_not_equals(env, expected, actual)                        \
  __z_assert_not_equals(env, PRIu64, expected, actual)

/* ============================================================================
 *  Assert numeric GE
 */
#define __z_assert_ge(env, frmt, actual, expected)                            \
  z_assert(env, ((actual) >= (expected)),                                     \
           "expected %"frmt" >= %"frmt, actual, expected)

#define z_assert_u32_ge(env, actual, expected)                                \
  __z_assert_ge(env, PRIu32, actual, expected)

#define z_assert_u64_ge(env, actual, expected)                                \
  __z_assert_ge(env, PRIu64, actual, expected)

/* ============================================================================
 *  Assert numeric GT
 */
#define __z_assert_gt(env, frmt, actual, expected)                            \
  z_assert(env, ((actual) > (expected)),                                      \
           "expected %"frmt" > %"frmt, actual, expected)

#define z_assert_u32_gt(env, actual, expected)                                \
  __z_assert_gt(env, PRIu32, actual, expected)

#define z_assert_u64_gt(env, actual, expected)                                \
  __z_assert_gt(env, PRIu64, actual, expected)

/* ============================================================================
 *  Assert numeric LE
 */
#define __z_assert_le(env, frmt, actual, expected)                            \
  z_assert(env, ((actual) <= (expected)),                                     \
           "expected %"frmt" <= %"frmt, actual, expected)

#define z_assert_u32_le(env, actual, expected)                                \
  __z_assert_le(env, PRIu32, actual, expected)

#define z_assert_u64_le(env, actual, expected)                                \
  __z_assert_le(env, PRIu64, actual, expected)

/* ============================================================================
 *  Assert numeric LT
 */
#define __z_assert_lt(env, frmt, actual, expected)                            \
  z_assert(env, ((actual) < (expected)),                                      \
           "expected %"frmt" < %"frmt, actual, expected)

#define z_assert_u32_lt(env, actual, expected)                                \
  __z_assert_lt(env, PRIu32, actual, expected)

#define z_assert_u64_lt(env, actual, expected)                                \
  __z_assert_lt(env, PRIu64, actual, expected)

/* ============================================================================
 *  Assert data equals
 */
#define __z_assert_memcmp(env, cond, expected, expsize, actual, actsize)      \
  do {                                                                        \
    char sexp[32], sact[32];                                                  \
    z_assert(env, cond,                                                       \
             "expected %s got %s",                                            \
             z_human_dblock(sexp, 32, expected, expsize),                     \
             z_human_dblock(sact, 32, actual, actsize));                      \
  } while (0)

#define z_assert_mem_equals(env, expected, expsize, actual, actsize)          \
  __z_assert_memcmp(env, z_mem_equals(expected, expsize, actual, actsize),    \
                    expected, expsize, actual, actsize)

#define z_assert_mem_not_equals(env, expected, expsize, actual, actsize)      \
  __z_assert_memcmp(env,                                                      \
                    z_mem_not_equals(expected, expsize, actual, actsize),     \
                    expected, expsize, actual, actsize)

#define z_assert_mem_lt(env, expected, expsize, actual, actsize)              \
  __z_assert_memcmp(env,                                                      \
                    z_mem_compare(expected, expsize, actual, actsize) < 0,    \
                    expected, expsize, actual, actsize)

#define z_assert_mem_le(env, expected, expsize, actual, actsize)              \
  __z_assert_memcmp(env,                                                      \
                    z_mem_compare(expected, expsize, actual, actsize) <= 0,   \
                    expected, expsize, actual, actsize)

#define z_assert_mem_gt(env, expected, expsize, actual, actsize)              \
  __z_assert_memcmp(env,                                                      \
                    z_mem_compare(expected, expsize, actual, actsize) > 0,    \
                    expected, expsize, actual, actsize)

#define z_assert_mem_ge(env, expected, expsize, actual, actsize)              \
  __z_assert_memcmp(env,                                                      \
                    z_mem_compare(expected, expsize, actual, actsize) >= 0,   \
                    expected, expsize, actual, actsize)

/* ============================================================================
 *  Assert test timeout
 */
#define z_utest_check_timeout_interrupt(env)                                  \
  do {                                                                        \
    if ((env)->fail_timeout > 0 &&                                            \
        z_timer_elapsed_millis(&((env)->timer)) > (env)->fail_timeout) {      \
      (env)->fail_line = __LINE__;                                            \
      snprintf((env)->fail_msg, sizeof((env)->fail_msg),                      \
               "killed by timeout %d", (env)->fail_timeout);                  \
      return;                                                                 \
    }                                                                         \
  } while (0)

/* ============================================================================
 *  Failure check
 */
#define z_utest_is_failed(env)        ((env)->fail_line > 0)
#define z_utest_check_failure(env)    if (z_utest_is_failed(env)) return;

/* ============================================================================
 *  Run helpers
 */
#define z_utest_run(func, timeout_msec)                                       \
  __z_utest_run(__FILE__, #func, func, timeout_msec)

#define z_utest_run_perf_size(func, size, num_runs)                           \
  __z_utest_run_perf_size(__FILE__, #func, func, size, num_runs)

int __z_utest_run (const char *file, const char *func_name,
                   z_utest_func_t func, int timeout_msec);

__Z_END_DECLS__

#endif /* !_Z_UTEST_H_ */
