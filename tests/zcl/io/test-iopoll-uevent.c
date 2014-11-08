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
#include <zcl/humans.h>
#include <zcl/debug.h>
#include <zcl/time.h>
#include <stdio.h>

#define __TEST_PERF    10000000

static uint64_t __nevents = 0;
static uint64_t __stime = 0;

static int __user_event (z_iopoll_entity_t *entity) {
#if __TEST_PERF
  if (__nevents++ < __TEST_PERF) {
    if ((__nevents & 1048575) == 0) {
      char buffer[64];
      float sec = (z_time_micros() - __stime) / 1000000.0f;
      z_human_dops(buffer, sizeof(buffer), __nevents / sec);
      Z_DEBUG("NEVENTS %.3fsec %s/sec", sec, buffer);
    }
    z_iopoll_notify(entity);
  } else {
    char buffer[64];
    float sec = (z_time_micros() - __stime) / 1000000.0f;
    z_human_dops(buffer, sizeof(buffer), __nevents / sec);
    Z_DEBUG("NEVENTS %.3fsec %s/sec", sec, buffer);
    z_global_context_stop();
  }

#else
  Z_DEBUG("TEST ENTITY %d UEVENT %u", entity->fd, entity->uflags8);
  if (entity->uflags8 < 10) {
    entity->uflags8++;
    z_iopoll_notify(entity);
  } else {
    z_iopoll_uevent(entity, 0);
    z_global_context_stop();
  }
#endif
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
  if (z_global_context_open_default(1)) {
    Z_LOG_FATAL("z_global_context_open(): failed\n");
    return(1);
  }

  z_iopoll_uevent_open(&entity, &__uevent_vtable, 0);
  entity.uflags8 = 0;

  z_iopoll_uevent(&entity, 1);

  __stime = z_time_micros();
  z_iopoll_notify(&entity);

  /* Start spinning... */
  z_global_context_poll(0);

  /* ...and we're done */
  z_global_context_close();
  return(0);
}
