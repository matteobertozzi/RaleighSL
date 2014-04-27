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
#include <zcl/mem.h>

/* ===========================================================================
 *  PUBLIC Histogram
 */
void z_histogram_init (z_histogram_t *self,
                       const uint64_t *bounds,
                       uint64_t *events,
                       uint32_t nbounds)
{
  self->bounds = bounds;
  self->events = events;
  self->nbuckets = nbounds;
  z_histogram_clear(self);
}

void z_histogram_clear (z_histogram_t *self) {
  self->min = 0xffffffffffffffff;
  self->max = 0;
  z_memzero(self->events, self->nbuckets * sizeof(uint64_t));
}

void z_histogram_add (z_histogram_t *self, uint64_t value) {
  const uint64_t *bound = self->bounds;
  uint64_t *events = self->events;

  while (value > *bound++)
    events += 1;
  *events += 1;

  self->sum += value;
  if (Z_UNLIKELY(value < self->min)) {
    self->min = value;
  }
  if (Z_UNLIKELY(value > self->max)) {
    self->max = value;
  }
}

uint64_t z_histogram_nevents (const z_histogram_t *self) {
  uint64_t nevents = 0;
  int i;
  for (i = 0; i < self->nbuckets; ++i)
    nevents += self->events[i];
  return(nevents);
}

double z_histogram_average (const z_histogram_t *self) {
  uint64_t nevents = z_histogram_nevents(self);
  return((nevents > 0) ? (((double)self->sum) / nevents) : 0);
}

double z_histogram_percentile (const z_histogram_t *self, double p) {
  uint64_t nevents = z_histogram_nevents(self);
  unsigned int buckets = self->nbuckets;
  double threshold = nevents * (p * 0.01);
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

void z_histogram_dump (const z_histogram_t *self, FILE *stream, z_human_u64_t key) {
  uint64_t min = 0xffffffffffffffff;
  uint64_t max = 0;
  char buffer[16];
  int i, j, step;

  for (i = 0; i < self->nbuckets; ++i) {
    min = z_min(min, self->events[i]);
    max = z_max(max, self->events[i]);
  }
  step = z_max(1, (max - min) / 60);

  fprintf(stream, "Min %s ", key(buffer, sizeof(buffer), self->min));
  fprintf(stream, "Max %s ", key(buffer, sizeof(buffer), self->max));
  fprintf(stream, "Avg %s ", key(buffer, sizeof(buffer), (uint64_t)z_histogram_average(self)));
  fprintf(stream, "\n--------------------------------------------------------------------------\n");

  for (i = 0; i < self->nbuckets - 1; ++i) {
    if (self->events[i] == 0) continue;

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
