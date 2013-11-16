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

#include <zcl/semaphore.h>
#include <zcl/time.h>

int z_semaphore_open (z_semaphore_t *self, int available) {
  z_wait_cond_alloc(&(self->wcond));
  z_mutex_alloc(&(self->mutex));
  self->available = available;
  return(0);
}

void z_semaphore_close (z_semaphore_t *self) {
  z_wait_cond_free(&(self->wcond));
  z_mutex_free(&(self->mutex));
}

void z_semaphore_acquire (z_semaphore_t *self, int n) {
  z_mutex_lock(&(self->mutex));
  while (n > self->available)
    z_wait_cond_wait(&(self->wcond), &(self->mutex), 0);
  self->available -= n;
  z_mutex_unlock(&(self->mutex));
}

int z_semaphore_try_acquire (z_semaphore_t *self, int n) {
  int acquired = 0;
  z_mutex_lock(&(self->mutex));
  if (n <= self->available) {
    self->available -= n;
    acquired = 1;
  }
  z_mutex_unlock(&(self->mutex));
  return(acquired);
}

int z_semaphore_try_acquire_timed (z_semaphore_t *self, int n, unsigned int msec) {
  uint64_t st, elapsed = 0;
  int acquired = 0;
  z_mutex_lock(&(self->mutex));
  st = z_time_millis();
  while (n > self->available && elapsed < msec) {
    z_wait_cond_wait(&(self->wcond), &(self->mutex), msec);
    elapsed = z_time_millis() - st;
  }
  if (n <= self->available) {
    self->available -= n;
    acquired = 1;
  }
  z_mutex_unlock(&(self->mutex));
  return(acquired);
}

void z_semaphore_release (z_semaphore_t *self, int n) {
  z_mutex_lock(&(self->mutex));
  self->available += n;
  z_wait_cond_broadcast(&(self->wcond));
  z_mutex_unlock(&(self->mutex));
}