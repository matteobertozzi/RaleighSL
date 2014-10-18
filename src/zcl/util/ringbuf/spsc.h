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

#ifndef _Z_SPMC_H_
#define _Z_SPMC_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

Z_TYPEDEF_STRUCT(z_spsc)

struct z_spsc {
  uint64_t next;
  uint64_t cached;
  uint64_t ring_size;
  uint64_t _pad_h[13];

  uint64_t cursor;
  uint64_t _pad_c[15];

  uint64_t gating_seq;
  uint64_t _pad_g[15];
};

void      z_spsc_init           (z_spsc_t *self, uint64_t ring_size);
int       z_spsc_try_next       (z_spsc_t *self, uint64_t *seqid);
void      z_spsc_publish        (z_spsc_t *self, uint64_t seqid);
int       z_spsc_try_acquire    (z_spsc_t *self, uint64_t *seqid);

__Z_END_DECLS__

#endif /* _Z_SPMC_H_ */
