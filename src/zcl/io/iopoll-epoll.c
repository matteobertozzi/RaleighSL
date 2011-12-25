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

#ifdef Z_IOPOLL_HAS_EPOLL

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include <zcl/socket.h>
#include <zcl/iopoll.h>
#include <zcl/memset.h>
#include <zcl/memcpy.h>

#define __IO_EPOLL_FDS                 512
#define __IO_EPOLL_STORE_ENTITY          1

#if __IO_EPOLL_STORE_ENTITY
    #define __epoll_entity_lookup(iopoll, event)                              \
        Z_IOPOLL_ENTITY((event)->data.ptr)
#else
    #define __epoll_entity_lookup(iopoll, event)                              \
        z_iopoll_lookup(iopoll, (event)->data.fd)
#endif

static int __epoll_init (z_iothread_t *iothread) {
    if ((iothread->data.fd = epoll_create(__IO_EPOLL_FDS)) < 0) {
        perror("epoll_create()");
        return(1);
    }
    return(0);
}

static void __epoll_uninit (z_iothread_t *iothread) {
    close(iothread->data.fd);
}

static int __epoll_set (z_iothread_t *iothread,
                        z_iopoll_entity_t *entity,
                        int op)
{
    struct epoll_event event;

    z_memzero(&event, sizeof(struct epoll_event));
    event.events = EPOLLRDHUP | EPOLLHUP;
#if __IO_EPOLL_STORE_ENTITY
    event.data.ptr = entity;
#else
    event.data.fd = entity->fd;
#endif

    z_socket_set_blocking(entity->fd, 0);

    if (entity->plug->read != NULL && z_iopoll_entity_is_readable(entity))
        event.events |= EPOLLIN;

    if (entity->plug->write != NULL && z_iopoll_entity_is_writeable(entity))
        event.events |= EPOLLOUT;

    if (epoll_ctl(iothread->data.fd, op, entity->fd, &event) < 0) {
        perror("epoll_ctl(EPOLL_CTL_ADD/MOD)");
        return(-1);
    }

    return(0);
}

static int __epoll_add (z_iothread_t *iothread, z_iopoll_entity_t *entity) {
    return(__epoll_set(iothread, entity, EPOLL_CTL_ADD));
}

static int __epoll_mod (z_iothread_t *iothread, z_iopoll_entity_t *entity) {
    return(__epoll_set(iothread, entity, EPOLL_CTL_MOD));
}

static int __epoll_remove (z_iothread_t *iothread, z_iopoll_entity_t *entity) {
    if (epoll_ctl(iothread->data.fd, EPOLL_CTL_DEL, entity->fd, NULL) < 0) {
        perror("epoll_ctl(EPOLL_CTL_DEL)");
        return(-1);
    }

    z_socket_set_blocking(entity->fd, 1);
    return(0);
}

static z_iopoll_entity_t *__epoll_process (z_iothread_t *iothread,
                                           const struct epoll_event *event)
{
    z_iopoll_entity_t *entity;

    entity = __epoll_entity_lookup(iothread->iopoll, event);

    if (event->events & EPOLLRDHUP || event->events & EPOLLHUP) {
        z_iopoll_remove(iothread->iopoll, entity);
        return(NULL);
    }

    if (event->events & EPOLLIN) {
        if (entity->plug->read(iothread->iopoll, entity) < 0)
            return(NULL);
    }

    if (event->events & EPOLLOUT)
        entity->plug->write(iothread->iopoll, entity);

    return(entity);
}

static int __epoll_poll (z_iothread_t *iothread) {
    struct epoll_event events[__IO_EPOLL_FDS];
    int timeout;
    int n;

    if (iothread->iopoll->timeout > 0)
        timeout = iothread->iopoll->timeout;
    else
        timeout = Z_IOPOLL_DEFAULT_TIMEOUT;

    while (z_iopoll_is_looping(iothread->iopoll)) {
        n = epoll_wait(iothread->data.fd, events, __IO_EPOLL_FDS, timeout);

        if (n < 0) {
            perror("epoll_wait()");
            continue;
        }

        while (n--)
            __epoll_process(iothread, &(events[n]));
    }

    return(0);
}

z_iopoll_plug_t z_iopoll_epoll = {
    .init       = __epoll_init,
    .uninit     = __epoll_uninit,
    .add        = __epoll_add,
    .mod        = __epoll_mod,
    .remove     = __epoll_remove,
    .poll       = __epoll_poll,
};

#endif /* Z_IOPOLL_HAS_EPOLL */
