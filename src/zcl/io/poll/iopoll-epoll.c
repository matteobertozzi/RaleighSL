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

#include <zcl/iopoll.h>
#include <zcl/time.h>

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#define __EPOLL_QSIZE           256

static int __epoll_open (z_iopoll_t *iopoll, z_iopoll_engine_t *engine) {
  if ((engine->data.fd = epoll_create(__EPOLL_QSIZE)) < 0) {
    perror("epoll_create()");
    return(-1);
  }
  return(0);
}

static void __epoll_close (z_iopoll_t *iopoll, z_iopoll_engine_t *engine) {
  close(engine->data.fd);
}

static int __epoll_insert (z_iopoll_t *iopoll,
                           z_iopoll_engine_t *engine,
                           z_iopoll_entity_t *entity)
{
  struct epoll_event event;
  int op;

  /* Initialize epoll events */
  event.events = EPOLLHUP | EPOLLRDHUP | EPOLLERR;
  if (entity->flags & Z_IOPOLL_READABLE) event.events |= EPOLLIN;
  if (entity->flags & Z_IOPOLL_WRITABLE) event.events |= EPOLLOUT;

  /* Epoll event data points to the entity */
  event.data.ptr = entity;

  op = (entity->flags & Z_IOPOLL_WATCHED) ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
  if (epoll_ctl(engine->data.fd, op, entity->fd, &event) < 0) {
    perror("epoll_ctl(EPOLL_CTL_ADD/MOD)");
    return(-1);
  }

  entity->flags |= Z_IOPOLL_WATCHED;
  return(0);
}

static int __epoll_remove (z_iopoll_t *iopoll,
                           z_iopoll_engine_t *engine,
                           z_iopoll_entity_t *entity)
{
  struct epoll_event event;

  event.events = 0;
  event.data.ptr = entity;
  if (epoll_ctl(engine->data.fd, EPOLL_CTL_DEL, entity->fd, &event) < 0) {
    perror("epoll_ctl(EPOLL_CTL_DEL)");
    return(-1);
  }

  entity->flags &= ~Z_IOPOLL_WATCHED;
  return(0);
}

static void __epoll_poll (z_iopoll_t *iopoll, z_iopoll_engine_t *engine) {
  struct epoll_event events[__EPOLL_QSIZE];
  z_timer_t timer;

  while (z_iopoll_is_looping(iopoll)) {
    struct epoll_event *e;
    int n;

    z_timer_start(&timer);
    n = epoll_wait(engine->data.fd, events, __EPOLL_QSIZE, iopoll->timeout);
    if (Z_UNLIKELY(n < 0)) {
      perror("epoll_wait()");
      continue;
    }
    z_timer_stop(&timer);
    z_iopoll_stats_add_events(engine, n, z_timer_micros(&timer));

    for (e = events; n--; ++e) {
      uint32_t eflags = (!!(e->events & EPOLLIN)) << Z_IOPOLL_READ |
          (!!(e->events & EPOLLOUT)) << Z_IOPOLL_WRITE |
          z_has(e->events, EPOLLRDHUP, EPOLLHUP, EPOLLERR) << Z_IOPOLL_HANG;
      z_iopoll_process(iopoll, engine, Z_IOPOLL_ENTITY(e->data.ptr), eflags);
    }
  }

  z_iopoll_stats_dump(engine);
}

const z_vtable_iopoll_t z_iopoll_epoll = {
  .open   = __epoll_open,
  .close  = __epoll_close,
  .insert = __epoll_insert,
  .remove = __epoll_remove,
  .poll   = __epoll_poll,
};

#endif /* Z_IOPOLL_HAS_EPOLL */
