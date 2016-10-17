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
#include <zcl/memutil.h>
#include <zcl/debug.h>

void z_histogram_init (z_histogram_t *self,
                       const uint64_t *bounds,
                       uint64_t *events,
                       unsigned int nbounds)
{
  self->bounds = bounds;
  self->events = events;
  z_histogram_clear(self, nbounds);
}

void z_histogram_merge (z_histogram_t *self, int n, ...) {
  va_list ap;
  int i;

  va_start(ap, n);
  for (i = 0; i < n; ++i) {
    const z_histogram_t *histo = va_arg(ap, z_histogram_t *);
    int j;
    Z_ASSERT(self->bounds == histo->bounds, "histo %p bounds do not match", histo);
    for (j = 0; histo->max > self->bounds[j]; ++j) {
      self->events[j] += histo->events[j];
    }
  }
  va_end(ap);
}

void z_histogram_clear (z_histogram_t *self, uint32_t nbounds) {
  z_memzero(self->events, nbounds * sizeof(uint64_t));
  self->max = 0;
}

void z_histogram_add (z_histogram_t *self, uint64_t value) {
  const uint64_t *bound = self->bounds;
  uint64_t *events = self->events;
  while (value > *bound++) {
    events += 1;
    Z_PREFETCH(bound);
  }
  *events += 1;
  if (Z_UNLIKELY(value > self->max)) {
    self->max = value;
  }
}

uint64_t z_histogram_nevents (const z_histogram_t *self) {
  uint64_t nevents = 0;
  int i;
  for (i = 0; self->max > self->bounds[i]; ++i) {
    nevents += self->events[i];
  }
  nevents += self->events[i];
  return(nevents);
}

double z_histogram_percentile (const z_histogram_t *self, const double p) {
  uint64_t nevents = z_histogram_nevents(self);
  double threshold = nevents * (p * 0.01);
  double sum = 0;
  int i;
  for (i = 0; self->max > self->bounds[i]; ++i) {
    sum += self->events[i];
    if (sum >= threshold) {
      // Scale linearly within this bucket
      double left_point = self->bounds[(i == 0) ? 0 : (i - 1)];
      double right_point = self->bounds[i];
      double left_sum = sum - self->events[i];
      double right_sum = sum;
      double pos = 0;
      double right_left_diff = right_sum - left_sum;
      if (right_left_diff != 0) {
        pos = (threshold - left_sum) / right_left_diff;
      }
      double r = left_point + (right_point - left_point) * pos;
      return((r > self->max) ? self->max : r);
    }
  }
  return(self->max);
}


void z_histogram_dump (const z_histogram_t *self, FILE *stream, z_human_dbl_t key) {
  uint64_t nevents = 0;
  uint64_t emax = 0;
  char buffer[16];
  double mult;
  int i;

  for (i = 0; self->max > self->bounds[i]; ++i) {
    emax = z_max(emax, self->events[i]);
    nevents += self->events[i];
  }
  emax = z_max(emax, self->events[i]);
  nevents += self->events[i];
  mult = 100.0 / nevents;

  fprintf(stream, "Max %s ", key(buffer, sizeof(buffer), self->max));
  fprintf(stream, "Events %s ", z_human_ops(buffer, sizeof(buffer), nevents));
  fprintf(stream, "\nPercentiles: ");
  fprintf(stream, "P50: %s ", key(buffer, sizeof(buffer), z_histogram_percentile(self, 50)));
  fprintf(stream, "P75: %s ", key(buffer, sizeof(buffer), z_histogram_percentile(self, 75)));
  fprintf(stream, "P99: %s ", key(buffer, sizeof(buffer), z_histogram_percentile(self, 99)));
  fprintf(stream, "\nPercentiles: ");
  fprintf(stream, "P99.9: %s ", key(buffer, sizeof(buffer), z_histogram_percentile(self, 99.9)));
  fprintf(stream, "P99.99: %s ", key(buffer, sizeof(buffer), z_histogram_percentile(self, 99.99)));
  fprintf(stream, "P99.999: %s ", key(buffer, sizeof(buffer), z_histogram_percentile(self, 99.999)));
  fprintf(stream, "\n--------------------------------------------------------------------------\n");

  for (i = 0; self->max > self->bounds[i]; ++i) {
    if (self->events[i] == 0) continue;

    fprintf(stream, "%10s (%5.2f%%) |", key(buffer, sizeof(buffer), self->bounds[i]),
                                        mult * self->events[i]);
    int barlen = (((float)self->events[i]) / emax) * 60;
    while (barlen--) fputc('=', stream);
    fprintf(stream, " (%"PRIu64" events)\n", self->events[i]);
  }

  if (self->events[i] > 0) {
    fprintf(stream, "%10s (%5.2f%%) |", key(buffer, sizeof(buffer), self->max),
                                        mult * self->events[i]);
    int barlen = (((float)self->events[i]) / emax) * 60;
    while (barlen--) fputc('=', stream);
    fprintf(stream, " (%"PRIu64" events)\n", self->events[i]);
  }
}