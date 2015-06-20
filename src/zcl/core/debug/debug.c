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

#include <zcl/humans.h>
#include <zcl/mutex.h>
#include <zcl/debug.h>
#include <zcl/time.h>

#include <execinfo.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

struct debug_conf {
  z_mutex_t lock;
  int log_level;
};

static struct debug_conf __current_debug_conf;

static const char *__log_level[7] = {
  "CODER", "FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE",
};

void __z_log (FILE *fp, int level, int errnum,
              const char *file, int line, const char *func,
              const char *format, ...)
{
  char datetime[32];
  void *trace[16];
  char **symbols;
  int i, size;
  va_list ap;

  if (z_in(level, Z_LOG_LEVEL_ERROR, Z_LOG_LEVEL_FATAL)) {
    size = backtrace(trace, 16);
    symbols = backtrace_symbols(trace, size);
  } else {
    symbols = NULL;
  }

  z_mutex_lock(&(__current_debug_conf.lock));
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
  if (symbols != NULL) {
    fprintf(stderr, "Obtained %d stack frames.\n", size);
    for (i = 0; i < size; i++) {
      fprintf(stderr, "%s\n", symbols[i]);
    }
  }
  z_mutex_unlock(&(__current_debug_conf.lock));

  if (symbols != NULL) {
    free(symbols);
  }
}

void __z_assert (const char *file, int line, const char *func,
                 int vcond, const char *condition,
                 const char *format, ...)
{
  char datetime[32];
  void *trace[16];
  char **symbols;
  int i, size;
  va_list ap;

  size = backtrace(trace, 16);
  symbols = backtrace_symbols(trace, size);

  z_mutex_lock(&(__current_debug_conf.lock));
  z_human_date(datetime, 32, z_time_micros());
  fprintf(stderr, "[%s] ASSERTION (%s) at %s:%d in function %s() failed. ",
          datetime, condition, file, line, func);

  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
  fprintf(stderr, "\n");

  if (symbols != NULL) {
    fprintf(stderr, "Obtained %d stack frames.\n", size);
    for (i = 0; i < size; i++) {
      fprintf(stderr, "%s\n", symbols[i]);
    }
  }
  z_mutex_unlock(&(__current_debug_conf.lock));

  if (symbols != NULL) {
    free(symbols);
  }
  abort();
}

int z_stack_trace (char *buffer, size_t bufsize, int levels) {
  void *trace[64];
  char **symbols;
  int i, size;
  int count;

  if (Z_UNLIKELY(levels == 0 || levels > 64)) {
    levels = 64;
  }

  size = backtrace(trace, levels);
  if ((symbols = backtrace_symbols(trace, size)) == NULL) {
    return(0);
  }

  count = snprintf(buffer, bufsize, "Obtained %d stack frames.\n", size);
  for (i = 0; i < size; i++) {
    count += snprintf(buffer + count, bufsize - count, "%s\n", symbols[i]);
  }

  free(symbols);
  return(count);
}

void z_dump_stack_trace (FILE *fp, int levels) {
  void *trace[64];
  char **symbols;
  int i, size;

  if (Z_UNLIKELY(levels == 0 || levels > 64))
    levels = 64;

  size = backtrace(trace, levels);
  if ((symbols = backtrace_symbols(trace, size)) == NULL) {
    perror("backtrace_symbols()");
    return;
  }

  z_mutex_lock(&(__current_debug_conf.lock));
  fprintf(fp, "Obtained %d stack frames.\n", size);
  for (i = 0; i < size; i++) {
    fprintf(fp, "%s\n", symbols[i]);
  }
  z_mutex_unlock(&(__current_debug_conf.lock));

  free(symbols);
}

void z_debug_open (void) {
  z_mutex_alloc(&(__current_debug_conf.lock));
  __current_debug_conf.log_level = Z_LOG_LEVEL_DEFAULT;
}

void z_debug_close (void) {
  z_mutex_free(&(__current_debug_conf.lock));
}

int z_debug_get_log_level (void) {
  return(__current_debug_conf.log_level);
}

void z_debug_set_log_level (int level) {
  z_mutex_lock(&(__current_debug_conf.lock));
  __current_debug_conf.log_level = level;
  z_mutex_unlock(&(__current_debug_conf.lock));
}
