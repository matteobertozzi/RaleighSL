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

#ifndef _Z_HISTOGRAM_H_
#define _Z_HISTOGRAM_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/object.h>

Z_TYPEDEF_STRUCT(z_histogram_interfaces)
Z_TYPEDEF_STRUCT(z_histogram)

#define Z_HISTOGRAM(x)            Z_CAST(z_histogram_t, x)

struct z_histogram_interfaces {
  Z_IMPLEMENT_TYPE
};

struct z_histogram {
  const uint64_t *bounds;
  unsigned int nbuckets;
  uint64_t *events;
  uint64_t nevents;
  uint64_t min;
  uint64_t max;
  uint64_t sum;
};

int     z_histogram_open          (z_histogram_t *self,
                                   const uint64_t *bounds,
                                   unsigned int nbounds);
void    z_histogram_close         (z_histogram_t *self);
void    z_histogram_clear         (z_histogram_t *self);
void    z_histogram_add           (z_histogram_t *self, uint64_t value);
double  z_histogram_average       (z_histogram_t *self);
double  z_histogram_percentile    (z_histogram_t *self, double p);

#define z_histogram_median(self)   z_histogram_percentile(self, 50.0)

__Z_END_DECLS__

#endif /* _Z_HISTOGRAM_H_ */
