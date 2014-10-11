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

#include <zcl/iopoll.h>
#include <zcl/debug.h>
#include <zcl/time.h>
#include <stdio.h>

static int __is_running = 1;

static int __user_event (z_iopoll_entity_t *entity) {
  Z_DEBUG("TEST ENTITY %d UEVENT %u", entity->fd, entity->uflags8);
  if (entity->uflags8 < 10) {
    entity->uflags8++;
    z_iopoll_notify(entity);
  } else {
    z_iopoll_uevent(entity, 0);
    __is_running = 0;
  }
  return(0);
}

const z_iopoll_entity_vtable_t __uevent_vtable = {
  .read    = NULL,
  .write   = NULL,
  .uevent  = __user_event,
  .timeout = NULL,
  .close   = NULL,
};

int main (int argc, char **argv) {
  z_iopoll_entity_t entity;

  /* Initialize global context */
  z_debug_open();

  /* Initialize I/O Poll */
  if (z_iopoll_open(1)) {
    Z_LOG_FATAL("z_iopoll_open(): failed\n");
    return(1);
  }

  z_iopoll_uevent_open(&entity, &__uevent_vtable, 0);
  entity.uflags8 = 0;

  z_iopoll_uevent(&entity, 1);
  z_iopoll_notify(&entity);

  /* Start spinning... */
  z_iopoll_poll(0, &__is_running);

  /* ...and we're done */
  z_iopoll_close();
  z_debug_close();
  return(0);
}
