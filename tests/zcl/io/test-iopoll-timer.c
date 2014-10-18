/*
 *   Copyright 2007-2013 Matteo Bertozzi
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

#include <zcl/global.h>
#include <zcl/debug.h>
#include <zcl/time.h>
#include <stdio.h>

static int __is_running = 1;

static int __timer_timeout (z_iopoll_entity_t *entity) {
  Z_DEBUG("TEST ENTITY %d TIMEOUT %u", entity->fd, entity->uflags8);
  if (entity->uflags8 < 100) {
    entity->uflags8 += 10;
    entity->uflags8 %= 80;
    z_iopoll_timer(entity, entity->uflags8 * 100);
  } else {
    entity->uflags8++;
    __is_running = (entity->uflags8 < 105);
  }
  return(0);
}

const z_iopoll_entity_vtable_t __timer_vtable = {
  .read    = NULL,
  .write   = NULL,
  .uevent  = NULL,
  .timeout = __timer_timeout,
  .close   = NULL,
};

int main (int argc, char **argv) {
  z_iopoll_entity_t entity[2];

  /* Initialize global context */
  if (z_global_context_open_default(1)) {
    Z_LOG_FATAL("z_iopoll_open(): failed\n");
    return(1);
  }

  z_iopoll_timer_open(&(entity[0]), &__timer_vtable, 0);
  z_iopoll_timer_open(&(entity[1]), &__timer_vtable, 1);

  entity[0].uflags8 = 10;
  entity[1].uflags8 = 100;

  z_iopoll_timer(entity + 0,  1000);
  z_iopoll_timer(entity + 1, 10000);

  /* Start spinning... */
  z_global_context_poll(0, &__is_running);

  /* ...and we're done */
  z_global_context_close();
  return(0);
}
