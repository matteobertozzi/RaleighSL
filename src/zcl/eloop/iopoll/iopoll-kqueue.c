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
#include <zcl/iopoll.h>
#ifdef Z_IOPOLL_HAS_KQUEUE

#include <zcl/iopoll.h>

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>

#include <zcl/iopoll.h>
#include <zcl/debug.h>
#include <zcl/timer.h>

static int __kqueue_open (z_iopoll_engine_t *self) {
  if ((self->data.fd = kqueue()) < 0) {
    Z_LOG_ERRNO_ERROR("kqueue()");
    return(1);
  }
  return(0);
}

static void __kqueue_close (z_iopoll_engine_t *self) {
  close(self->data.fd);
}

static int __kqueue_add (z_iopoll_engine_t *self,
                         z_iopoll_entity_t *entity)
{
  struct kevent events[2];
  int nevents = 0;

  if (entity->iflags8 & Z_IOPOLL_WRITABLE) {
    EV_SET(&(events[nevents++]), entity->fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, entity);
  } else {
    EV_SET(&(events[nevents++]), entity->fd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, entity);
  }

  if (entity->iflags8 & Z_IOPOLL_READABLE) {
    EV_SET(&(events[nevents++]), entity->fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, entity);
  } else {
    EV_SET(&(events[nevents++]), entity->fd, EVFILT_READ, EV_ADD | EV_DISABLE, 0, 0, entity);
  }

  if (Z_UNLIKELY(nevents && kevent(self->data.fd, events, nevents, NULL, 0, NULL) < 0)) {
    Z_LOG_ERRNO_WARN("kevent(EV_ADD)");
    return(-1);
  }

  entity->iflags8 |= Z_IOPOLL_WATCHED;
  return(0);
}

static int __kqueue_remove (z_iopoll_engine_t *self,
                            z_iopoll_entity_t *entity)
{
  struct kevent events[2];
  int nevents = 0;

  if (entity->iflags8 & Z_IOPOLL_WRITABLE) {
    EV_SET(&(events[nevents++]), entity->fd, EVFILT_WRITE, EV_DELETE, 0, 0, entity);
  }

  if (entity->iflags8 & Z_IOPOLL_READABLE) {
    EV_SET(&(events[nevents++]), entity->fd, EVFILT_READ, EV_DELETE, 0, 0, entity);
  }

  if (Z_UNLIKELY(nevents && kevent(self->data.fd, events, nevents, NULL, 0, NULL) < 0)) {
    Z_LOG_ERRNO_WARN("kevent(EV_DELETE)");
    return(-1);
  }

  entity->iflags8 &= ~Z_IOPOLL_WATCHED;
  return(0);
}

static int __kqueue_timer (z_iopoll_engine_t *self,
                           z_iopoll_entity_t *entity,
                           uint32_t msec)
{
  struct kevent event;

  if (msec > 0) {
    EV_SET(&event, entity->fd, EVFILT_TIMER, EV_ADD | EV_ENABLE, 0, msec, entity);
  } else {
    EV_SET(&event, entity->fd, EVFILT_TIMER, EV_DELETE, 0, 0, entity);
  }

  if (Z_UNLIKELY(kevent(self->data.fd, &event, 1, NULL, 0, NULL) < 0)) {
    Z_LOG_ERRNO_WARN("kevent(EV_ADD)");
    return(-1);
  }
  return(0);
}

static int __kqueue_uevent (z_iopoll_engine_t *self,
                            z_iopoll_entity_t *entity,
                            int enable)
{
  struct kevent event;

  if (enable) {
    EV_SET(&event, entity->fd, EVFILT_USER, EV_ADD | EV_DISPATCH, 0, 0, entity);
  } else {
    EV_SET(&event, entity->fd, EVFILT_USER, EV_DELETE, 0, 0, entity);
  }

  if (Z_UNLIKELY(kevent(self->data.fd, &event, 1, NULL, 0, NULL) < 0)) {
    Z_LOG_ERRNO_WARN("kevent(EV_ADD)");
    return(-1);
  }
  return(0);
}

static int __kqueue_notify (z_iopoll_engine_t *self,
                            z_iopoll_entity_t *entity)
{
  struct kevent event;
  EV_SET(&event, entity->fd, EVFILT_USER, EV_ENABLE, NOTE_TRIGGER, 0, entity);
  if (Z_UNLIKELY(kevent(self->data.fd, &event, 1, NULL, 0, NULL) < 0)) {
    Z_LOG_ERRNO_WARN("kevent(EV_ADD)");
    return(-1);
  }
  return(0);
}

static void __kqueue_poll (z_iopoll_engine_t *self) {
  struct kevent events[Z_IOPOLL_MAX_EVENTS];
  struct kevent *e;
  z_timer_t timer;
  int n;

  z_timer_start(&timer);
  n = kevent(self->data.fd, NULL, 0, events, Z_IOPOLL_MAX_EVENTS, NULL);
  if (Z_UNLIKELY(n < 0)) {
    Z_LOG_ERRNO_WARN("kevent()");
    return;
  }
  z_timer_stop(&timer);
  z_iopoll_stats_add_events(self, n, z_timer_nanos(&timer));

  z_timer_reset(&timer);
  for (e = events; n--; ++e) {
    uint32_t eflags =
        (e->filter == EVFILT_READ)  << Z_IOPOLL_READ  |
        (e->filter == EVFILT_WRITE) << Z_IOPOLL_WRITE |
        (e->filter == EVFILT_TIMER) << Z_IOPOLL_TIMER |
        (e->filter == EVFILT_USER)  << Z_IOPOLL_USER  |
        (e->flags & EV_EOF && e->filter == EVFILT_READ) << Z_IOPOLL_HANG;
    z_iopoll_process(self, Z_IOPOLL_ENTITY(e->udata), eflags);
  }
  z_timer_stop(&timer);
  z_iopoll_stats_add_active(self, z_timer_nanos(&timer));
}

const z_iopoll_vtable_t z_iopoll_kqueue = {
  .add    = __kqueue_add,
  .remove = __kqueue_remove,
  .timer  = __kqueue_timer,
  .uevent = __kqueue_uevent,
  .notify = __kqueue_notify,
  .open   = __kqueue_open,
  .close  = __kqueue_close,
  .poll   = __kqueue_poll,
};

#endif /* Z_IOPOLL_HAS_KQUEUE */
