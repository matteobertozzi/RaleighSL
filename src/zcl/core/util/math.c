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

#include <math.h>

#include <zcl/math.h>

unsigned int z_ilog2 (unsigned int n) {
  unsigned int lg2 = 0U;
  if (n & (n - 1)) lg2 += 1;
  if (n >> 16) { lg2 += 16; n >>= 16; }
  if (n >>  8) { lg2 +=  8; n >>= 8; }
  if (n >>  4) { lg2 +=  4; n >>= 4; }
  if (n >>  2) { lg2 +=  2; n >>= 2; }
  if (n >>  1) { lg2 +=  1; }
  return(lg2);
}

double z_percentile (const double *values, size_t size, double percent) {
  double k = (size - 1) * percent;
  double f = floor(k);
  double c = floor(k);
  double d0 = values[(size_t)f] * (c - k);
  double d1 = values[(size_t)c] * (k - f);
  return(d0 + d1);
}

unsigned int z_rand (unsigned int *seed) {
  unsigned int next = *seed;
  unsigned int result;

  next *= 1103515245;
  next += 12345;
  result = (next >> 16) & 2047;

  next *= 1103515245;
  next += 12345;
  result <<= 10;
  result ^= (next >> 16) & 1023;

  next *= 1103515245;
  next += 12345;
  result <<= 10;
  result ^= (next >> 16) & 1023;

  *seed = next;
  return(result);
}
