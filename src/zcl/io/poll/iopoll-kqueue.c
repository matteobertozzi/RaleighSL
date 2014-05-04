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

#include <zcl/config.h>
#ifdef Z_IOPOLL_HAS_KQUEUE

#include "iopoll_private.h"
#include <zcl/iopoll.h>
#include <zcl/debug.h>
#include <zcl/time.h>

#include <sys/event.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#define __KQUEUE_QSIZE           (256)
#define __KQUEUE_TIMEOUT_SEC     (2)

static int __kqueue_open (z_iopoll_engine_t *engine) {
  if ((engine->data.fd = kqueue()) < 0) {
    Z_LOG_ERRNO_ERROR("kqueue()");
    return(1);
  }
  return(0);
}

static void __kqueue_close (z_iopoll_engine_t *engine) {
  close(engine->data.fd);
}

static int __kqueue_insert (z_iopoll_engine_t *engine,
                            z_iopoll_entity_t *entity)
{
  struct kevent events[2];
  struct kevent *pevents;
  int nevents = 0;

  if (entity->iflags8 & Z_IOPOLL_WRITABLE) {
    EV_SET(&(events[1]), entity->fd, EVFILT_WRITE, EV_ADD, 0, 0, entity);
    pevents = &(events[1]);
    ++nevents;
  }

  if (entity->iflags8 & Z_IOPOLL_READABLE) {
    EV_SET(&(events[0]), entity->fd, EVFILT_READ, EV_ADD, 0, 0, entity);
    pevents = &(events[0]);
    ++nevents;
  }

  if (Z_UNLIKELY(nevents && kevent(engine->data.fd, pevents, nevents, NULL, 0, NULL) < 0)) {
    Z_LOG_ERRNO_WARN("kevent(EV_ADD)");
    return(-1);
  }

  entity->iflags8 |= Z_IOPOLL_WATCHED;
  return(0);
}

static int __kqueue_remove (z_iopoll_engine_t *engine,
                            z_iopoll_entity_t *entity)
{
  struct kevent events[2];
  struct kevent *pevents;
  int nevents = 0;

  if (entity->iflags8 & Z_IOPOLL_WRITABLE) {
    EV_SET(&(events[1]), entity->fd, EVFILT_WRITE, EV_DELETE, 0, 0, entity);
    pevents = &(events[1]);
    ++nevents;
  }

  if (entity->iflags8 & Z_IOPOLL_READABLE) {
    EV_SET(&(events[0]), entity->fd, EVFILT_READ, EV_DELETE, 0, 0, entity);
    pevents = &(events[0]);
    ++nevents;
  }

  if (Z_UNLIKELY(nevents && kevent(engine->data.fd, pevents, nevents, NULL, 0, NULL) < 0)) {
    Z_LOG_ERRNO_WARN("kevent(EV_DELETE)");
    return(-1);
  }

  entity->iflags8 &= ~Z_IOPOLL_WATCHED;
  return(0);
}

static void __kqueue_poll (z_iopoll_engine_t *engine) {
  const uint8_t *is_running = z_iopoll_is_running();
  struct kevent events[__KQUEUE_QSIZE];
  struct timespec timeout;
  z_timer_t timer;

  timeout.tv_sec  = __KQUEUE_TIMEOUT_SEC;
  timeout.tv_nsec = 0;
  while (*is_running) {
    struct kevent *e;
    int n;

    z_timer_start(&timer);
    n = kevent(engine->data.fd, NULL, 0, events, __KQUEUE_QSIZE, &timeout);
    if (Z_UNLIKELY(n < 0)) {
      Z_LOG_ERRNO_WARN("kevent()");
      continue;
    }
    z_timer_stop(&timer);
    z_iopoll_stats_add_events(engine, n, z_timer_micros(&timer));

    for (e = events; n--; ++e) {
      uint32_t eflags = (e->filter == EVFILT_READ) << Z_IOPOLL_READ |
          (e->filter == EVFILT_WRITE) << Z_IOPOLL_WRITE |
          (e->flags & EV_EOF && e->filter == EVFILT_READ) << Z_IOPOLL_HANG;
      z_iopoll_process(engine, Z_IOPOLL_ENTITY(e->udata), eflags);
    }
  }
}

const z_vtable_iopoll_t z_iopoll_kqueue = {
  .open   = __kqueue_open,
  .close  = __kqueue_close,
  .insert = __kqueue_insert,
  .remove = __kqueue_remove,
  .poll   = __kqueue_poll,
};

#endif /* Z_IOPOLL_HAS_KQUEUE */
