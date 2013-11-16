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

#include <stdio.h>

#include <zcl/humans.h>

char *z_human_dsize (char *buffer, size_t bufsize, double size) {
  if (size >= (1ULL << 60))
    snprintf(buffer, bufsize, "%.2fEiB", size / Z_EB(1ULL));
  else if (size >= (1ULL << 50))
    snprintf(buffer, bufsize, "%.2fPiB", size / Z_PB(1ULL));
  else if (size >= (1ULL << 40))
    snprintf(buffer, bufsize, "%.2fTiB", size / Z_TB(1ULL));
  else if (size >= (1ULL << 30))
    snprintf(buffer, bufsize, "%.2fGiB", size / Z_GB(1ULL));
  else if (size >= (1ULL << 20))
    snprintf(buffer, bufsize, "%.2fMiB", size / Z_MB(1ULL));
  else if (size >= (1ULL << 10))
    snprintf(buffer, bufsize, "%.2fKiB", size / Z_KB(1ULL));
  else
    snprintf(buffer, bufsize, "%.0fbytes", size);
  return(buffer);
}

char *z_human_size (char *buffer, size_t bufsize, uint64_t size) {
  return(z_human_dsize(buffer, bufsize, size));
}

char *z_human_time (char *buffer, size_t bufsize, uint64_t msec) {
  if (msec >= 60000000)
    snprintf(buffer, bufsize, "%.2fmin", msec / 60000000.0);
  else if (msec >= 1000000)
    snprintf(buffer, bufsize, "%.2fsec", msec / 1000000.0);
  else if (msec >= 1000U)
    snprintf(buffer, bufsize, "%.2fmsec", msec / 1000.0);
  else
    snprintf(buffer, bufsize, "%"PRIu64"usec", msec);
  return(buffer);
}
