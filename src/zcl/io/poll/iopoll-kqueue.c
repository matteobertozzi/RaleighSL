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

#include <zcl/iopoll.h>
#include <zcl/time.h>

#include <sys/event.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#define __KQUEUE_QSIZE           256

static int __kqueue_open (z_iopoll_t *iopoll, z_iopoll_engine_t *engine) {
  if ((engine->data.fd = kqueue()) < 0) {
    perror("kqueue()");
    return(1);
  }
  return(0);
}

static void __kqueue_close (z_iopoll_t *iopoll, z_iopoll_engine_t *engine) {
  close(engine->data.fd);
}

static int __kqueue_insert (z_iopoll_t *iopoll,
                            z_iopoll_engine_t *engine,
                            z_iopoll_entity_t *entity)
{
  struct kevent event;

  if (entity->flags & Z_IOPOLL_READABLE) {
    EV_SET(&event, entity->fd, EVFILT_READ, EV_ADD, 0, 0, entity);
    if (kevent(engine->data.fd, &event, 1, NULL, 0, NULL) < 0) {
      perror("kevent(EV_ADD|EVFILT_READ)");
      return(-1);
    }
  }

  if (entity->flags & Z_IOPOLL_WRITABLE) {
    EV_SET(&event, entity->fd, EVFILT_WRITE, EV_ADD, 0, 0, entity);
    if (kevent(engine->data.fd, &event, 1, NULL, 0, NULL) < 0) {
      perror("kevent(EV_ADD|EVFILT_WRITE)");
      return(-1);
    }
  }

  entity->flags |= Z_IOPOLL_WATCHED;
  return(0);
}

static int __kqueue_remove (z_iopoll_t *iopoll,
                            z_iopoll_engine_t *engine,
                            z_iopoll_entity_t *entity)
{
  struct kevent event;

  if (entity->flags & Z_IOPOLL_READABLE) {
    EV_SET(&event, entity->fd, EVFILT_READ, EV_DELETE, 0, 0, entity);
    if (kevent(engine->data.fd, &event, 1, NULL, 0, NULL) < 0) {
      perror("kevent(EV_DELETE|EVFILT_READ)");
      return(-1);
    }
  }

  if (entity->flags & Z_IOPOLL_WRITABLE) {
    EV_SET(&event, entity->fd, EVFILT_WRITE, EV_DELETE, 0, 0, entity);
    if (kevent(engine->data.fd, &event, 1, NULL, 0, NULL) < 0) {
      perror("kevent(EV_DELETE|EVFILT_WRITE)");
      return(-1);
    }
  }

  entity->flags &= ~Z_IOPOLL_WATCHED;
  return(0);
}

static void __kqueue_poll (z_iopoll_t *iopoll, z_iopoll_engine_t *engine) {
  struct kevent events[__KQUEUE_QSIZE];
  struct timespec timeout;
  z_timer_t timer;

  if (iopoll->timeout > 0) {
    timeout.tv_sec = iopoll->timeout / 1000;
    timeout.tv_nsec = 0;
  } else {
    timeout.tv_sec =  5;
    timeout.tv_nsec = 0;
  }

  while (z_iopoll_is_looping(iopoll)) {
    struct kevent *e;
    int n;

    z_timer_start(&timer);
    n = kevent(engine->data.fd, NULL, 0, events, __KQUEUE_QSIZE, &timeout);
    if (Z_UNLIKELY(n < 0)) {
      perror("kevent()");
      continue;
    }
    z_timer_stop(&timer);
    z_iopoll_stats_add_events(engine, n, z_timer_micros(&timer));

    for (e = events; n--; ++e) {
      uint32_t eflags = (e->filter == EVFILT_READ) << Z_IOPOLL_READ |
          (e->filter == EVFILT_WRITE) << Z_IOPOLL_WRITE |
          (e->flags & EV_EOF && e->filter == EVFILT_READ) << Z_IOPOLL_HANG;
      z_iopoll_process(iopoll, engine, Z_IOPOLL_ENTITY(e->udata), eflags);
    }
  }

  z_iopoll_stats_dump(engine);
}

const z_vtable_iopoll_t z_iopoll_kqueue = {
  .open   = __kqueue_open,
  .close  = __kqueue_close,
  .insert = __kqueue_insert,
  .remove = __kqueue_remove,
  .poll   = __kqueue_poll,
};

#endif /* Z_IOPOLL_HAS_KQUEUE */
