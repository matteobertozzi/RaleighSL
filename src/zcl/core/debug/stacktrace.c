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

#include <zcl/stacktrace.h>

#include <execinfo.h>

int z_stack_trace (char *buffer, size_t bufsize, int levels) {
  void *trace[64];
  char **symbols;
  int i, size;
  int count;

  if (Z_UNLIKELY(levels <= 0 || levels > 64)) {
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

void z_stack_trace_dump (FILE *fp, int levels) {
  void *trace[64];
  char **symbols;
  int i, size;

  if (Z_UNLIKELY(levels <= 0 || levels > 64)) {
    levels = 64;
  }

  size = backtrace(trace, levels);
  if ((symbols = backtrace_symbols(trace, size)) == NULL) {
    perror("backtrace_symbols()");
    return;
  }

  fprintf(fp, "Obtained %d stack frames.\n", size);
  for (i = 0; i < size; i++) {
    fprintf(fp, "%s\n", symbols[i]);
  }

  free(symbols);
}
