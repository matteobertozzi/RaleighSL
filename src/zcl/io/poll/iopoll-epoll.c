/*
 *   Copyright 2011-2012 Matteo Bertozzi
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

static int __epoll_open (z_iopoll_t *iopoll) {
    int efd;

    if ((efd = epoll_create(512)) < 0) {
        perror("epoll_create()");
        return(-1);
    }

    iopoll->data.fd = efd;
    return(0);
}

static void __epoll_close (z_iopoll_t *iopoll) {
    close(iopoll->data.fd);
}

static int __epoll_insert (z_iopoll_t *iopoll, z_iopoll_entity_t *entity) {
    struct epoll_event event;
    int op;

    /* Initialize epoll events */
    event.events = EPOLLHUP | EPOLLRDHUP | EPOLLERR;
    if (entity->flags & Z_IOPOLL_READABLE) event.events |= EPOLLIN;
    if (entity->flags & Z_IOPOLL_WRITABLE) event.events |= EPOLLOUT;

    /* Epoll event data points to the entity */
    event.data.ptr = entity;

    op = (entity->flags & Z_IOPOLL_WATCHED) ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    if (epoll_ctl(iopoll->data.fd, op, entity->fd, &event) < 0) {
        perror("epoll_ctl(EPOLL_CTL_ADD/MOD)");
        return(-1);
    }

    entity->flags |= Z_IOPOLL_WATCHED;
    return(0);
}

static int __epoll_remove (z_iopoll_t *iopoll, z_iopoll_entity_t *entity) {
    struct epoll_event event;

    event.events = 0;
    event.data.ptr = entity;
    if (epoll_ctl(iopoll->data.fd, EPOLL_CTL_DEL, entity->fd, &event) < 0) {
        perror("epoll_ctl(EPOLL_CTL_DEL)");
        return(-1);
    }

    entity->flags &= ~Z_IOPOLL_WATCHED;
    return(0);
}

static void __epoll_process (z_iopoll_t *iopoll,
                             z_iopoll_entity_t *entity,
                             uint32_t events)
{
    if (Z_UNLIKELY(z_has(events, EPOLLRDHUP, EPOLLHUP, EPOLLERR))) {
        z_iopoll_remove(iopoll, entity);
        entity->vtable->close(iopoll, entity);
        return;
    }

    if (events & EPOLLIN) {
        iopoll->stats.read_events++;
        if (entity->vtable->read(iopoll, entity) < 0) {
            z_iopoll_remove(iopoll, entity);
            entity->vtable->close(iopoll, entity);
            return;
        }
    }

    if (events & EPOLLOUT) {
        iopoll->stats.write_events++;
        if (entity->vtable->write(iopoll, entity) < 0) {
            z_iopoll_remove(iopoll, entity);
            entity->vtable->close(iopoll, entity);
            return;
        }
    }
}

static void __epoll_poll (z_iopoll_t *iopoll) {
    struct epoll_event events[512];
    struct epoll_event *e;
    z_timer_t timer;
    int n;

    while (z_iopoll_is_looping(iopoll)) {
        z_timer_start(&timer);
        n = epoll_wait(iopoll->data.fd, events, 512, iopoll->timeout);
        z_timer_stop(&timer);

        iopoll->stats.max_events = z_max(iopoll->stats.max_events, n);
        iopoll->stats.avg_iowait = z_avg(iopoll->stats.avg_iowait, z_timer_micros(&timer));

        if (Z_UNLIKELY(n < 0)) {
            perror("epoll_wait()");
            continue;
        }

        z_timer_start(&timer);
        for (e = events; n--; ++e) {
            __epoll_process(iopoll, Z_IOPOLL_ENTITY(e->data.ptr), e->events);
        }
        z_timer_stop(&timer);
        iopoll->stats.avg_ioprocess = z_avg(iopoll->stats.avg_ioprocess, z_timer_micros(&timer));
    }
}

const z_vtable_iopoll_t z_iopoll_epoll = {
    .open   = __epoll_open,
    .close  = __epoll_close,
    .insert = __epoll_insert,
    .remove = __epoll_remove,
    .poll   = __epoll_poll,
};

#endif /* Z_IOPOLL_HAS_EPOLL */
