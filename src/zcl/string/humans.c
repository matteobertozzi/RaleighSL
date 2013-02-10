/*
 *   Copyright 2011-2013 Matteo Bertozzi
 *
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

char *z_human_size (char *buffer, unsigned int bufsize, double size) {
  if (size >= (1ULL << 60))
    snprintf(buffer, bufsize, "%.2fEiB", size / (1ULL << 60));
  else if (size >= (1ULL << 50))
    snprintf(buffer, bufsize, "%.2fPiB", size / (1ULL << 50));
  else if (size >= (1ULL << 40))
    snprintf(buffer, bufsize, "%.2fTiB", size / (1ULL << 40));
  else if (size >= (1ULL << 30))
    snprintf(buffer, bufsize, "%.2fGiB", size / (1ULL << 30));
  else if (size >= (1ULL << 20))
    snprintf(buffer, bufsize, "%.2fMiB", size / (1ULL << 20));
  else if (size >= (1ULL << 10))
    snprintf(buffer, bufsize, "%.2fKiB", size / (1ULL << 10));
  else
    snprintf(buffer, bufsize, "%.0fbytes", size);
  return(buffer);
}

char *z_human_time (char *buffer, unsigned int bufsize, uint64_t time) {
  if (time >= 60000000)
    snprintf(buffer, bufsize, "%.2fmin", time / 60000000.0);
  else if (time >= 1000000)
    snprintf(buffer, bufsize, "%.2fsec", time / 1000000.0);
  else if (time >= 1000U)
    snprintf(buffer, bufsize, "%.2fmsec", time / 1000.0);
  else
    snprintf(buffer, bufsize, "%luusec", time);
  return(buffer);
}
