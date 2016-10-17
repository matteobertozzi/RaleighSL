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

#include <zcl/debug.h>
#include <zcl/humans.h>
#include <zcl/stacktrace.h>
#include <zcl/time.h>

#include <stdarg.h>
#include <string.h>

struct z_debug_conf {
  int log_level;
};

static struct z_debug_conf __current_debug_conf = {
  .log_level = Z_LOG_LEVEL_DEFAULT
};

void z_debug_open (void) {
  __current_debug_conf.log_level = Z_LOG_LEVEL_DEFAULT;
}

void z_debug_close (void) {
}

int z_debug_get_log_level (void) {
  return(__current_debug_conf.log_level);
}

void z_debug_set_log_level (int level) {
  __current_debug_conf.log_level = level;
}

/* ===========================================================================
 *  Internal helpers
 */
static const char *__log_level[7] = {
  "CODER", "FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE",
};

void __z_log (FILE *fp, int level, int errnum,
              const char *file, int line, const char *func,
              const char *format, ...)
{
  char datetime[32];
  va_list ap;

  z_human_date(datetime, 32, z_time_micros());
  fprintf(fp, "[%s] %-5s %s:%d %s() - ",
          datetime, __log_level[level], file, line, func);

  va_start(ap, format);
  vfprintf(fp, format, ap);
  va_end(ap);

  if (errnum != 0) {
    fprintf(fp, ": errno %d %s", errnum, strerror(errnum));
  }

  fprintf(fp, "\n");
  if (z_in(level, Z_LOG_LEVEL_ERROR, Z_LOG_LEVEL_FATAL)) {
    z_stack_trace_dump(fp, 0);
  }
}

void __z_assert (const char *file, int line, const char *func,
                 int vcond, const char *condition,
                 const char *format, ...)
{
  char datetime[32];
  va_list ap;

  z_human_date(datetime, 32, z_time_micros());
  fprintf(stderr, "[%s] ASSERTION (%s) at %s:%d in function %s() failed. ",
          datetime, condition, file, line, func);

  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
  fprintf(stderr, "\n");
  z_stack_trace_dump(stderr, 0);

  abort();
}
