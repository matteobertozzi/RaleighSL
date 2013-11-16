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
static int __histogram_get_bucket (const z_histogram_t *self, uint64_t value) {
  int buckets = self->nbuckets - 1;
  int i;
  for (i = 0; i < buckets; ++i) {
    if (value <= self->bounds[i])
      return(i);
  }
  return(i);
}

/* ===========================================================================
 *  PUBLIC Histogram
 */
void z_histogram_open (z_histogram_t *self,
                       const uint64_t *bounds,
                       uint64_t *events,
                       unsigned int nbounds)
{
  self->bounds = bounds;
  self->events = events;
  self->nbuckets = nbounds + 1;
  z_histogram_clear(self);
}

void z_histogram_close (z_histogram_t *self) {
}

void z_histogram_clear (z_histogram_t *self) {
  self->nevents = 0;
  self->min = 0xffffffffffffffff;
  self->max = 0;
  self->sum = 0;
  z_memzero(self->events, self->nbuckets * sizeof(uint64_t));
}

void z_histogram_add (z_histogram_t *self, uint64_t value) {
  int index = __histogram_get_bucket(self, value);
  ++self->events[index];
  if (Z_UNLIKELY(self->nevents++ == 0)) {
    self->min = value;
    self->max = value;
  } else if (Z_UNLIKELY(value < self->min)) {
    self->min = value;
  } else if (Z_UNLIKELY(value > self->max)) {
    self->max = value;
  }
  self->sum += value;
}

double z_histogram_average (z_histogram_t *self) {
  return((self->nevents > 0) ? (((double)self->sum) / self->nevents) : 0);
}

double z_histogram_percentile (z_histogram_t *self, double p) {
  unsigned int buckets = self->nbuckets;
  double threshold = self->nevents * (p * 0.01);
  double sum = 0;
  int i;
  for (i = 0; i < buckets; ++i) {
    sum += self->events[i];
    if (sum >= threshold) {
      /* TODO: Fix me */
      return self->bounds[i];
    }
  }
  return(self->max);
}

void z_histogram_dump (z_histogram_t *self, FILE *stream, z_human_u64_t key) {
  uint64_t min = 0xffffffffffffffff;
  uint64_t max = 0;
  char buffer[16];
  int i, j, step;

  if (self->nevents == 0) {
    fprintf(stream, "No Data\n");
    return;
  }

  for (i = 0; i < self->nbuckets; ++i) {
    min = z_min(min, self->events[i]);
    max = z_max(max, self->events[i]);
  }
  step = z_max(1, (max - min) / 60);

  fprintf(stream, "Min %s ", key(buffer, sizeof(buffer), self->min));
  fprintf(stream, "Max %s ", key(buffer, sizeof(buffer), self->max));
  fprintf(stream, "Avg %s ", key(buffer, sizeof(buffer), (uint64_t)z_histogram_average(self)));
  fprintf(stream, "\n----------------------------------------------------------------------\n");

  for (i = 0; i < self->nbuckets - 1; ++i) {
    fprintf(stream, "%10s |", key(buffer, sizeof(buffer), self->bounds[i]));
    for (j = 0; j < self->events[i]; j += step) fputc('=', stream);
    fprintf(stream, " (%"PRIu64" events)\n", self->events[i]);
  }

  if (self->events[i] > 0) {
    fprintf(stream, "%10s |", key(buffer, sizeof(buffer), self->max));
    for (j = 0; j < self->events[i]; j += step) fputc('=', stream);
    fprintf(stream, " (%"PRIu64" events)\n", self->events[i]);
  }
}
