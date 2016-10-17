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

#include <zcl/codel.h>
#include <zcl/time.h>

void z_codel_init (z_codel_t *self, uint16_t interval, uint16_t target_delay) {
  self->next_ts = z_time_millis();
  self->reset_delay = 1;
  self->overloaded = 0;
  self->interval = interval;
  self->min_delay = 0;
  self->target_delay = target_delay;
}

int z_codel_is_overloaded (z_codel_t *self, const uint64_t callq_timestamp) {
  const uint64_t now = z_time_millis();
  const uint64_t call_delay = now - callq_timestamp;

  if (now > self->next_ts && !self->reset_delay) {
    self->reset_delay = 1;
    self->next_ts = now + self->interval;
    self->overloaded = (self->min_delay > self->target_delay);
  }

  if (self->reset_delay) {
    self->reset_delay = 0;
    self->min_delay = call_delay;
    return 0;
  }

  if (call_delay < self->min_delay) {
    self->min_delay = call_delay;
  }

  const uint32_t slough_timeout = 2 * self->target_delay;
  return self->overloaded && call_delay > slough_timeout;
}
