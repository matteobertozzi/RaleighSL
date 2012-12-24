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

#ifndef _Z_IOPOLL_H_
#define _Z_IOPOLL_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/opaque.h>

#define Z_IOPOLL_WATCHED            (1U << 0)
#define Z_IOPOLL_READABLE           (1U << 1)
#define Z_IOPOLL_WRITABLE           (1U << 2)

#define Z_IOPOLL_ENTITY_FD(x)       (Z_IOPOLL_ENTITY(x)->fd)
#define Z_IOPOLL_ENTITY(x)          Z_CAST(z_iopoll_entity_t, x)
#define Z_IOPOLL(x)                 Z_CAST(z_iopoll_t, x)

#define __Z_IOPOLL_ENTITY__         z_iopoll_entity_t __io_entity__;

Z_TYPEDEF_STRUCT(z_vtable_iopoll_entity)
Z_TYPEDEF_STRUCT(z_vtable_iopoll)
Z_TYPEDEF_STRUCT(z_iopoll_entity)
Z_TYPEDEF_STRUCT(z_iopoll_stats)
Z_TYPEDEF_STRUCT(z_iopoll)

struct z_vtable_iopoll_entity {
    int     (*read)     (z_iopoll_t *iopoll,
                         z_iopoll_entity_t *entity);
    int     (*write)    (z_iopoll_t *iopoll,
                         z_iopoll_entity_t *entity);
    void    (*close)    (z_iopoll_t *iopoll,
                         z_iopoll_entity_t *entity);
};

struct z_vtable_iopoll {
    int     (*open)     (z_iopoll_t *self);
    void    (*close)    (z_iopoll_t *self);
    int     (*insert)   (z_iopoll_t *self,
                         z_iopoll_entity_t *entry);
    int     (*remove)   (z_iopoll_t *self,
                         z_iopoll_entity_t *entry);
    void    (*poll)     (z_iopoll_t *self);
};

struct z_iopoll_entity {
    const z_vtable_iopoll_entity_t *vtable;
    unsigned int flags;
    int fd;
};

struct z_iopoll_stats {
    uint64_t read_events;
    uint64_t write_events;
    uint64_t avg_ioprocess;
    uint64_t avg_iowait;
    unsigned int max_events;
};

struct z_iopoll {
    const z_vtable_iopoll_t *vtable;
    const int *      is_looping;
    z_iopoll_stats_t stats;
    int              timeout;
    z_opaque_t       data;
};

#ifdef Z_IOPOLL_HAS_EPOLL
extern const z_vtable_iopoll_t z_iopoll_epoll;
#endif /* Z_IOPOLL_HAS_EPOLL */

#define z_iopoll_is_looping(iopoll)                                          \
  (((iopoll)->is_looping == NULL) || *((iopoll)->is_looping))

int   z_iopoll_open         (z_iopoll_t *iopoll,
                             const z_vtable_iopoll_t *vtable);
void  z_iopoll_close        (z_iopoll_t *iopoll);
int   z_iopoll_add          (z_iopoll_t *iopoll,
                             z_iopoll_entity_t *entity);
int   z_iopoll_remove       (z_iopoll_t *iopoll,
                             z_iopoll_entity_t *entity);
void  z_iopoll_poll         (z_iopoll_t *iopoll,
                             const int *is_looping,
                             int timeout);

void  z_iopoll_entity_init  (z_iopoll_entity_t *entity,
                             const z_vtable_iopoll_entity_t *vtable,
                             int fd);
void  z_iopoll_set_writable (z_iopoll_t *iopoll,
                             z_iopoll_entity_t *entity,
                             int writable);

__Z_END_DECLS__

#endif /* !_Z_IOPOLL_H_ */
