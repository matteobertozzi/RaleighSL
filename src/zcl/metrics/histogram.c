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

#include <zcl/histogram.h>
#include <zcl/global.h>
#include <zcl/string.h>

/* ===========================================================================
 *  PRIVATE Histogram methods
 */
static int __histogram_get_bucket (z_histogram_t *self, uint64_t value) {
#if 0
  int high = self->nbuckets - 1;
  int low = 0;

  while (low < high) {
    int mid = low + ((high - low) >> 1);
    if (value <= self->bounds[mid]) {
      high = mid;
    } else {
      low = mid + 1;
    }
  }

  return(high);
#else
  int buckets = self->nbuckets - 1;
  int i;
  for (i = 0; i < buckets; ++i) {
    if (value <= self->bounds[i])
      return(i);
  }
  return(i);
#endif
}

/* ===========================================================================
 *  PUBLIC Histogram
 */
int z_histogram_open (z_histogram_t *self, const uint64_t *bounds, unsigned int nbounds) {
  unsigned int buckets;

  self->bounds = bounds;
  buckets = nbounds + 1;

  if ((self->events = z_memory_array_alloc(z_global_memory(), uint64_t, buckets)) == NULL)
    return(1);

  z_memzero(self->events, buckets * sizeof(uint64_t));
  self->nbuckets = buckets & 0xffffffff;
  self->nevents = 0;
  self->min = 0xffffffffffffffff;
  self->max = 0;
  self->sum = 0;
  return(0);
}

void z_histogram_close (z_histogram_t *self) {
  z_memory_array_free(z_global_memory(), self->events);
}

void z_histogram_clear (z_histogram_t *self) {
  self->nevents = 0;
  self->min = 0xffffffffffffffff;
  self->max = 0;
  self->sum = 0;
  z_memzero(self, self->nbuckets * sizeof(uint64_t));
}

void z_histogram_add (z_histogram_t *self, uint64_t value) {
  if (value < self->min) self->min = value;
  if (value > self->max) self->max = value;
  self->sum += value;
  self->nevents++;
  self->events[__histogram_get_bucket(self, value)]++;
}

double z_histogram_average (z_histogram_t *self) {
  return((self->nevents > 0) ? (((double)self->sum) / self->nevents) : 0);
}


double z_histogram_percentile (z_histogram_t *self, double p) {
  unsigned int buckets = self->nbuckets;
  double threshold = self->nevents * (p * 0.01);
  double sum = 0;
  for (int i = 0; i < buckets; ++i) {
    sum += self->events[i];
    if (sum >= threshold) {
      /* TODO: Fix me */
      return self->bounds[i];
    }
  }
  return(self->max);
}
