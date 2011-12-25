/*
 *   Copyright 2011 Matteo Bertozzi
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

#ifdef Z_IOPOLL_HAS_KQUEUE

#include <sys/event.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>

#include <zcl/iopoll.h>
#include <zcl/memset.h>
#include <zcl/memcpy.h>

#define __IO_KQUEUE_EVENTS                  256

#define __kevent_entity(event)              Z_IOPOLL_ENTITY((event)->udata)

enum z_ioevent_type {
    Z_IOPOLL_EVENT_READ  = 1,
    Z_IOPOLL_EVENT_WRITE = 2,
    Z_IOPOLL_EVENT_HANGUP = 4,
};

typedef struct z_iopoll_event {
    z_iopoll_entity_t *entity;
    unsigned int       events;
} z_iopoll_event_t;

static int __iopoll_event_process (z_iopoll_t *iopoll,
                                   const z_iopoll_event_t *event)
{
    z_iopoll_entity_t *entity = event->entity;

    if (event->events & Z_IOPOLL_EVENT_HANGUP) {
        z_iopoll_remove(iopoll, event->entity);
        return(-1);
    }

    if (event->events & Z_IOPOLL_EVENT_READ) {
        if (entity->plug->read(iopoll, entity) < 0)
            return(-2);
    }

    if (event->events & Z_IOPOLL_EVENT_WRITE) {
        entity->plug->write(iopoll, entity);
    }

    return(0);
}

static int __kqueue_init (z_iothread_t *iothread) {
    if ((iothread->data.fd = kqueue()) < 0) {
        perror("kqueue()");
        return(1);
    }
    return(0);
}

static void __kqueue_uninit (z_iothread_t *iothread) {
    close(iothread->data.fd);
}

static int __kqueue_set (z_iothread_t *iothread, z_iopoll_entity_t *entity) {
    struct kevent event;

    if (entity->plug->read != NULL && z_iopoll_entity_is_readable(entity)) {
        EV_SET(&event, entity->fd, EVFILT_READ, EV_ADD, 0, 0, entity);
        if (kevent(iothread->data.fd, &event, 1, NULL, 0, NULL) < 0) {
            perror("kevent(EV_ADD|EVFILT_READ)");
            return(-1);
        }
    }

    if (entity->plug->write != NULL && z_iopoll_entity_is_writeable(entity)) {
        EV_SET(&event, entity->fd, EVFILT_WRITE, EV_ADD, 0, 0, entity);
        if (kevent(iothread->data.fd, &event, 1, NULL, 0, NULL) < 0) {
            perror("kevent(EV_ADD|EVFILT_WRITE)");
            return(-1);
        }
    }

    return(0);
}

static int __kqueue_remove (z_iothread_t *iothread, z_iopoll_entity_t *entity) {
    struct kevent event;

    if (entity->plug->read != NULL && z_iopoll_entity_is_readable(entity)) {
        EV_SET(&event, entity->fd, EVFILT_READ, EV_DELETE, 0, 0, entity);
        if (kevent(iothread->data.fd, &event, 1, NULL, 0, NULL) < 0) {
            perror("kevent(EV_DELETE|EVFILT_READ)");
            return(-1);
        }
    }

    if (entity->plug->write != NULL && z_iopoll_entity_is_writeable(entity)) {
        EV_SET(&event, entity->fd, EVFILT_WRITE, EV_DELETE, 0, 0, entity);
        if (kevent(iothread->data.fd, &event, 1, NULL, 0, NULL) < 0) {
            perror("kevent(EV_DELETE|EVFILT_WRITE)");
            return(-1);
        }
    }

    return(0);
}

static int __kqueue_merge_events (z_iopoll_event_t *ioevents,
                                  const struct kevent *events,
                                  int nevents)
{
    z_iopoll_entity_t *entity;
    z_iopoll_event_t *ioev;
    int nioevents;
    int i, j;

    nioevents = 0;
    for (i = 0; i < nevents; ++i) {
        ioev = NULL;
        entity = __kevent_entity(&(events[i]));
        for (j = 0; j < nioevents; ++j) {
            if (ioevents[j].entity == entity) {
                ioev = &(ioevents[j]);
                break;
            }
        }

        if (ioev == NULL) {
            ioev = &(ioevents[nioevents++]);
            ioev->entity = entity;
            ioev->events = 0;
        }

        if (events[i].flags & EV_EOF && events[i].filter == EVFILT_READ)
            ioev->events |= Z_IOPOLL_EVENT_HANGUP;

        if (events[i].filter == EVFILT_READ && entity->plug->read != NULL)
            ioev->events |= Z_IOPOLL_EVENT_READ;

        if (events[i].filter == EVFILT_WRITE && entity->plug->write != NULL)
            ioev->events |= Z_IOPOLL_EVENT_WRITE;
    }

    return(nioevents);
}

static int __kqueue_poll (z_iothread_t *iothread) {
    z_iopoll_event_t ioevents[__IO_KQUEUE_EVENTS];
    struct kevent events[__IO_KQUEUE_EVENTS];
    struct timespec timeout;
    int n;

    if (iothread->iopoll->timeout > 0) {
        timeout.tv_sec = iothread->iopoll->timeout / 1000;
        timeout.tv_nsec = 0;
    } else {
        timeout.tv_sec = Z_IOPOLL_DEFAULT_TIMEOUT / 1000;
        timeout.tv_nsec = 0;
    }

    while (z_iopoll_is_looping(iothread->iopoll)) {
        n = kevent(iothread->data.fd, NULL, 0,
                   events, __IO_KQUEUE_EVENTS, &timeout);

        if (n < 0) {
            perror("kevent()");
            continue;
        }

        n = __kqueue_merge_events(ioevents, events, n);
        while (n--)
            __iopoll_event_process(iothread->iopoll, &(ioevents[n]));
    }

    return(0);
}

z_iopoll_plug_t z_iopoll_kqueue = {
    .init       = __kqueue_init,
    .uninit     = __kqueue_uninit,
    .add        = __kqueue_set,
    .mod        = __kqueue_set,
    .remove     = __kqueue_remove,
    .poll       = __kqueue_poll,
};

#endif /* Z_IOPOLL_HAS_KQUEUE */
