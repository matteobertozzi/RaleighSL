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

static void __utest_env_init (z_utest_env_t *env, int timeout_msec) {
  /* failure fields */
  env->fail_msg[0] = '\0';
  env->fail_timeout = timeout_msec;
  env->fail_line = -1;

  /* perf fields */
  env->perf_bytes = 0;
  env->perf_ops = 0;
}

int __z_utest_run (const char *file, const char *func_name,
                   z_utest_func_t func, int timeout_msec)
{
  z_utest_env_t env;
  char hbuf[64];

  __utest_env_init(&env, timeout_msec);

  z_timer_start(&env.timer);
  func(&env);
  z_timer_stop(&env.timer);

  z_human_time(hbuf, sizeof(hbuf), z_timer_nanos(&env.timer));
  if (z_utest_is_failed(&env)) {
    fprintf(stderr, "%s failed after %s - %s() line %d: %s\n",
                    file, hbuf, func_name, env.fail_line, env.fail_msg);
    if (0 && env.stack_trace[0] != '\0') {
      fprintf(stderr, "%s\n", env.stack_trace);
    }
    return(1);
  }

  fprintf(stderr, "%s succeded after %s - %s()", file, hbuf, func_name);

  if (env.perf_bytes > 0) {
    z_human_dsize(hbuf, sizeof(hbuf), env.perf_bytes / z_timer_secs(&env.timer));
    fprintf(stderr, " - perf-bytes %s/sec", hbuf);

    z_human_dops(hbuf, sizeof(hbuf), env.perf_bytes);
    fprintf(stderr, " (data %s)", hbuf);
  }

  if (env.perf_ops > 0) {
    z_human_dops(hbuf, sizeof(hbuf), env.perf_ops / z_timer_secs(&env.timer));
    fprintf(stderr, " - perf-ops %s/sec", hbuf);

    z_human_dops(hbuf, sizeof(hbuf), env.perf_ops);
    fprintf(stderr, " (ops %s)", hbuf);
  }

  fprintf(stderr, "\n");
  return(0);
}
