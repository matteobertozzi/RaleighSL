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

#include <zcl/string.h>
#include <zcl/iopoll.h>

int z_iopoll_open (z_iopoll_t *iopoll, const z_vtable_iopoll_t *vtable) {
    z_memzero(iopoll, sizeof(z_iopoll_t));
    if (vtable == NULL) {
#ifdef Z_IOPOLL_HAS_EPOLL
        iopoll->vtable = &z_iopoll_epoll;
#endif
    } else {
        iopoll->vtable = vtable;
    }
    return(iopoll->vtable->open(iopoll));
}

void z_iopoll_close (z_iopoll_t *iopoll) {
    iopoll->vtable->close(iopoll);
}

int z_iopoll_add (z_iopoll_t *iopoll, z_iopoll_entity_t *entity) {
    return(iopoll->vtable->insert(iopoll, entity));
}

int z_iopoll_remove (z_iopoll_t *iopoll, z_iopoll_entity_t *entity) {
    return(iopoll->vtable->remove(iopoll, entity));
}

void z_iopoll_poll (z_iopoll_t *iopoll, const int *is_looping, int timeout) {
    iopoll->is_looping = is_looping;
    iopoll->timeout = timeout;
    iopoll->vtable->poll(iopoll);
}

void z_iopoll_entity_init (z_iopoll_entity_t *entity,
                           const z_vtable_iopoll_entity_t *vtable,
                           int fd)
{
    entity->vtable = vtable;
    entity->flags = Z_IOPOLL_READABLE;
    entity->fd = fd;
}

void z_iopoll_set_writable (z_iopoll_t *iopoll,
                            z_iopoll_entity_t *entity,
                            int writable)
{
    if (writable != !!(entity->flags & Z_IOPOLL_WRITABLE)) {
        entity->flags ^= Z_IOPOLL_WRITABLE;
        z_iopoll_add(iopoll, entity);
    }
}
