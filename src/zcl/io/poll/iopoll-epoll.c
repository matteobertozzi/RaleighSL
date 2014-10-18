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
#ifdef Z_IOPOLL_HAS_EPOLL

#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>

#include <zcl/global.h>
#include <zcl/iopoll.h>
#include <zcl/debug.h>
#include <zcl/time.h>

#define __EPOLL_QSIZE           (256)

static int __epoll_open (z_iopoll_engine_t *self) {
  if ((self->data.fd = epoll_create(__EPOLL_QSIZE)) < 0) {
    Z_LOG_ERRNO_ERROR("epoll()");
    return(1);
  }
  return(0);
}

static void __epoll_close (z_iopoll_engine_t *self) {
  close(self->data.fd);
}

static int __epoll_add (z_iopoll_engine_t *self, z_iopoll_entity_t *entity) {
  struct epoll_event event;
  int op;

  /* Initialize epoll events */
  event.events  = EPOLLHUP | EPOLLRDHUP | EPOLLERR;
  event.events |= EPOLLIN  * !!(entity->iflags8 & Z_IOPOLL_READABLE);
  event.events |= EPOLLOUT * !!(entity->iflags8 & Z_IOPOLL_WRITABLE);
  Z_LOG_DEBUG("entity %d read %d write %d", entity->fd, event.events & EPOLLIN, event.events & EPOLLOUT);

  /* Epoll event data points to the entity */
  event.data.ptr = entity;

  op = (entity->iflags8 & Z_IOPOLL_WATCHED) ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
  if (Z_UNLIKELY(epoll_ctl(self->data.fd, op, entity->fd, &event) < 0)) {
    Z_LOG_ERRNO_WARN("epoll_ctl(EPOLL_CTL_ADD/MOD)");
    return(-1);
  }

  entity->iflags8 |= Z_IOPOLL_WATCHED;
  return(0);
}

static int __epoll_remove (z_iopoll_engine_t *self, z_iopoll_entity_t *entity) {
  struct epoll_event event;

  event.events = 0;
  event.data.ptr = entity;
  if (Z_UNLIKELY(epoll_ctl(self->data.fd, EPOLL_CTL_DEL, entity->fd, &event) < 0)) {
    Z_LOG_ERRNO_WARN("epoll_ctl(EPOLL_CTL_DEL)");
    return(-1);
  }

  entity->iflags8 &= ~Z_IOPOLL_WATCHED;
  return(0);
}

static int __epoll_timer (z_iopoll_engine_t *self,
                          z_iopoll_entity_t *entity,
                          uint32_t msec)
{
  if (msec > 0) {
    struct epoll_event event;
    struct itimerspec ts;
    struct timespec now;
    int op;

    if (entity->fd < 0) {
      entity->fd = timerfd_create(CLOCK_REALTIME, 0);
      if (Z_UNLIKELY(entity->fd < 0)) {
        Z_LOG_ERRNO_WARN("timerfd_create()");
        return(-1);
      }
      op = EPOLL_CTL_ADD;
    } else {
      op = EPOLL_CTL_MOD;
    }

    clock_gettime(CLOCK_REALTIME, &now);
    ts.it_interval.tv_sec  = (msec / 1000);
    ts.it_interval.tv_nsec = (msec % 1000) * 1000000;
    ts.it_value.tv_sec  = now.tv_sec + ts.it_interval.tv_sec;
    ts.it_value.tv_nsec = now.tv_nsec + ts.it_interval.tv_nsec;
    if (Z_UNLIKELY(timerfd_settime(entity->fd, TFD_TIMER_ABSTIME, &ts, NULL) < 0)) {
      Z_LOG_ERRNO_WARN("timerfd_settime()");
      return(-1);
    }

    /* Initialize epoll event */
    event.events = EPOLLET | EPOLLIN;
    event.data.ptr = entity;
    if (Z_UNLIKELY(epoll_ctl(self->data.fd, op, entity->fd, &event) < 0)) {
      Z_LOG_ERRNO_WARN("epoll_ctl(EPOLL_CTL_ADD/MOD)");
      return(-1);
    }
  } else if (entity->fd >= 0) {
    close(entity->fd);
    entity->fd = -1;
  }
  return(0);
}

static int __epoll_uevent (z_iopoll_engine_t *self,
                           z_iopoll_entity_t *entity,
                           int enable)
{
  if (enable && entity->fd < 0) {
    struct epoll_event event;

    entity->fd = eventfd(0, 0);
    if (Z_UNLIKELY(entity->fd < 0)) {
      Z_LOG_ERRNO_WARN("eventfd()");
      return(-1);
    }

    /* Initialize epoll event */
    event.events = EPOLLET | EPOLLIN;
    event.data.ptr = entity;
    if (Z_UNLIKELY(epoll_ctl(self->data.fd, EPOLL_CTL_ADD, entity->fd, &event) < 0)) {
      Z_LOG_ERRNO_WARN("epoll_ctl(EPOLL_CTL_ADD)");
      return(-1);
    }
  } else if (entity->fd >= 0) {
    close(entity->fd);
    entity->fd = -1;
  }
  return(0);
}

static int __epoll_notify (z_iopoll_engine_t *self,
                           z_iopoll_entity_t *entity)
{
  uint64_t v = 1;
  Z_ASSERT(entity->fd >= 0, "unregistered user event");
  if (Z_UNLIKELY(write(entity->fd, &v, 8) != 8)) {
    Z_LOG_ERRNO_WARN("write(eventfd)");
  }
  return(0);
}

static void __epoll_poll (z_iopoll_engine_t *self) {
  const int *is_running = z_global_is_running();
  struct epoll_event events[__EPOLL_QSIZE];
  z_timer_t timer;

  while (*is_running) {
    const struct epoll_event *e;
    int n;

    z_timer_start(&timer);
    n = epoll_wait(self->data.fd, events, __EPOLL_QSIZE, -1);
    if (Z_UNLIKELY(n < 0)) {
      Z_LOG_ERRNO_WARN("epoll_wait()");
      continue;
    }
    z_timer_stop(&timer);
    z_iopoll_stats_add_events(self, n, z_timer_micros(&timer));

    for (e = events; n--; ++e) {
      z_iopoll_entity_t *entity = Z_IOPOLL_ENTITY(e->data.ptr);
      uint32_t eflags = 0;
      switch (entity->iflags8) {
        case Z_IOPOLL_TIMEOUT:
        case Z_IOPOLL_UEVENT:
          if (e->events & EPOLLIN) {
            uint64_t v;
            if (Z_LIKELY(read(entity->fd, &v, 8) == 8)) {
              eflags = entity->iflags8;
            } else {
              Z_LOG_ERRNO_WARN("read(eventfd/timerfd)");
            }
          }
          break;
        default:
          eflags = (!!(e->events & EPOLLIN))  << Z_IOPOLL_READ |
                   (!!(e->events & EPOLLOUT)) << Z_IOPOLL_WRITE |
                   z_has(e->events, EPOLLRDHUP, EPOLLHUP, EPOLLERR) << Z_IOPOLL_HANG;
          break;
      }
      z_iopoll_process(self, entity, eflags);
    }
  }
}

const z_iopoll_vtable_t z_iopoll_epoll = {
  .add    = __epoll_add,
  .remove = __epoll_remove,
  .timer  = __epoll_timer,
  .uevent = __epoll_uevent,
  .notify = __epoll_notify,
  .open   = __epoll_open,
  .close  = __epoll_close,
  .poll   = __epoll_poll,
};

#endif /* Z_IOPOLL_HAS_EPOLL */
