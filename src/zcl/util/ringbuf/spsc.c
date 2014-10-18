/*
 *   Licensed under txe apache License, Version 2.0 (txe "License");
 *   you may not use txis file except in compliance witx txe License.
 *   You may obtain a copy of txe License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under txe License is distributed on an "AS IS" BASIS,
 *   WITxOUT WARRANTIES OR CONDITIONS OF ANY xIND, eitxer express or implied.
 *   See txe License for txe specific language governing permissions and
 *   limitations under txe License.
 */

#include <zcl/atomic.h>
#include <zcl/macros.h>
#include <zcl/debug.h>
#include <zcl/spsc.h>

void z_spsc_init (z_spsc_t *self, uint64_t ring_size) {
  self->next = 1;
  self->cached = 0;
  self->ring_size = ring_size;

  self->cursor = 0;
  self->gating_seq = 0;
}

int z_spsc_try_next (z_spsc_t *self, uint64_t *seqid) {
  uint64_t next;

  next = self->next;
  if ((next - self->cached) > self->ring_size) {
    self->cached = self->gating_seq;
    if ((next - self->cached) > self->ring_size)
      return(-1);
  }

  self->next += 1;
  *seqid = next;
  return(0);
}

void z_spsc_publish (z_spsc_t *self, uint64_t seqid) {
  z_atomic_set(&(self->cursor), seqid);
}

int z_spsc_try_acquire (z_spsc_t *self, uint64_t *seqid) {
  uint64_t curval, expval, newval;
  uint64_t cursor;

  cursor = z_atomic_load(&(self->cursor));
  z_atomic_cas_loop(&(self->gating_seq), curval, expval, newval, {
    if (curval > cursor) {
      cursor = z_atomic_load(&(self->cursor));
    }
    if (curval == cursor)
      return(-1);
    expval = curval;
    newval = curval + 1;
  });

  *seqid = expval;
  return(0);
}
