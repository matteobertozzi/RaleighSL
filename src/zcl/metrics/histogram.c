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

#include <zcl/histogram.h>
#include <zcl/string.h>

/* ===========================================================================
 *  PRIVATE Histogram macros
 */
#define __histogram_buckets(histo)          z_object_flags(histo).uflags32

/* ===========================================================================
 *  PRIVATE Histogram methods
 */
static int __histogram_get_bucket (z_histogram_t *histo, uint64_t value) {
#if 0
    int high = __histogram_buckets(histo) - 1;
    int low = 0;

    while (low < high) {
        int mid = low + ((high - low) >> 1);
        if (value <= histo->bounds[mid]) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }

    return(high);
#else
    int buckets = __histogram_buckets(histo) - 1;
    int i;
    for (i = 0; i < buckets; ++i) {
        if (value <= histo->bounds[i])
            return(i);
    }
    return(i);
#endif
}

/* ===========================================================================
 *  PUBLIC Histogram
 */
void z_histogram_clear (z_histogram_t *histo) {
    histo->nevents = 0;
    histo->min = 0xffffffffffffffff;
    histo->max = 0;
    histo->sum = 0;
    z_memzero(histo, __histogram_buckets(histo) * sizeof(uint64_t));
}

void z_histogram_add (z_histogram_t *histo, uint64_t value) {
    if (value < histo->min) histo->min = value;
    if (value > histo->max) histo->max = value;
    histo->sum += value;
    histo->nevents++;
    histo->events[__histogram_get_bucket(histo, value)]++;
}

double z_histogram_average (z_histogram_t *histo) {
    return((histo->nevents > 0) ? (((double)histo->sum) / histo->nevents) : 0);
}

double z_histogram_percentile (z_histogram_t *histo, double p) {
    unsigned int buckets = __histogram_buckets(histo);
    double threshold = histo->nevents * (p * 0.01);
    double sum = 0;
    for (int i = 0; i < buckets; ++i) {
        sum += histo->events[i];
        if (sum >= threshold) {
            /* TODO: Fix me */
            double left_point = (i == 0) ? 0 : histo->bounds[i - 1];
            double right_point = (i < buckets) ? histo->bounds[i] : histo->max;
            double left_sum = sum - histo->events[i];
            double pos = (threshold - left_sum) / (sum - left_sum);
            double r = left_point + (right_point - left_point) * pos;
            return((r < histo->min) ? histo->min : (r > histo->max) ? histo->max : r);
        }
    }
    return(histo->max);
}

/* ===========================================================================
 *  INTERFACE Type methods
 */
static int __histogram_ctor (void *self, z_memory_t *memory, va_list args) {
    z_histogram_t *histo = Z_HISTOGRAM(self);
    unsigned int buckets;

    histo->bounds = va_arg(args, const uint64_t *);
    buckets = va_arg(args, unsigned int) + 1;

    if ((histo->events = z_object_array_zalloc(histo, uint64_t, buckets)) == NULL)
        return(1);

    __histogram_buckets(histo) = buckets & 0xffffffff;
    histo->nevents = 0;
    histo->min = 0xffffffffffffffff;
    histo->max = 0;
    histo->sum = 0;

    return(0);
}

static void __histogram_dtor (void *self) {
    z_histogram_t *histo = Z_HISTOGRAM(self);
    z_object_array_free(histo, histo->events);
}

/* ===========================================================================
 *  Histogram vtables
 */
static const z_vtable_type_t __histogram_type = {
    .name = "Histogram",
    .size = sizeof(z_histogram_t),
    .ctor = __histogram_ctor,
    .dtor = __histogram_dtor,
};

static const z_histogram_interfaces_t __histogram_interfaces = {
    .type = &__histogram_type,
};

/* ===========================================================================
 *  PUBLIC Histogram constructor/destructor
 */
z_histogram_t *z_histogram_alloc (z_histogram_t *self,
                                  z_memory_t *memory,
                                  const uint64_t *bounds,
                                  unsigned int nbounds)
{
    return(z_object_alloc(self, &__histogram_interfaces,
                          memory, bounds, nbounds));
}

void z_histogram_free (z_histogram_t *self) {
    z_object_free(self);
}

