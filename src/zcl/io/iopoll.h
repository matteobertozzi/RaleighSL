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

#ifndef _Z_IOPOLL_H_
#define _Z_IOPOLL_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/hashtable.h>
#include <zcl/object.h>
#include <zcl/types.h>

#define Z_IOPOLL_USE_IOTHREAD
#ifdef Z_IOPOLL_USE_IOTHREAD
    #include <zcl/thread.h>
    #ifndef Z_IOPOLL_NTHREADS
        #define Z_IOPOLL_NTHREADS       (8)
    #endif /* !Z_IOPOLL_NTHREADS */
#endif /* Z_IOPOLL_USE_IOTHREAD */

#define Z_IOPOLL_ENTITY_TYPE                z_iopoll_entity_t __base__;
#define Z_IOPOLL_ENTITY(x)                  Z_CAST(z_iopoll_entity_t, x)
#define Z_IOPOLL_ENTITY_FD(x)               (Z_IOPOLL_ENTITY(x)->fd)
#define Z_IOTHREAD(x)                       Z_CAST(z_iothread_t, x)
#define Z_IOPOLL(x)                         Z_CAST(z_iopoll_t, x)

Z_TYPEDEF_CONST_STRUCT(z_iopoll_entity_plug)
Z_TYPEDEF_CONST_STRUCT(z_iopoll_plug)
Z_TYPEDEF_STRUCT(z_iopoll_entity)
Z_TYPEDEF_STRUCT(z_iothread)
Z_TYPEDEF_STRUCT(z_iopoll)

typedef int (*z_iopoll_read_t)              (z_iopoll_t *iopoll,
                                             z_iopoll_entity_t *entity);
typedef int (*z_iopoll_write_t)             (z_iopoll_t *iopoll,
                                             z_iopoll_entity_t *entity);

struct z_iopoll_entity_plug {
    z_iopoll_write_t write;
    z_iopoll_read_t  read;
    z_mem_free_t     free;
};

struct z_iopoll_entity {
    z_iopoll_entity_plug_t *plug;
#ifdef Z_IOPOLL_USE_IOTHREAD
    z_iothread_t *iothread;
#endif /* Z_IOPOLL_USE_IOTHREAD */
    uint64_t flags;
    int fd;
};

struct z_iothread {
    z_iopoll_t * iopoll;
    z_data_t     data;
#ifdef Z_IOPOLL_USE_IOTHREAD
    z_thread_t   thread;
    unsigned int nfds;
#endif /* Z_IOPOLL_USE_IOTHREAD */
};

struct z_iopoll_plug {
    int     (*init)         (z_iothread_t *iothread);
    void    (*uninit)       (z_iothread_t *iothread);

    int     (*add)          (z_iothread_t *iothread,
                             z_iopoll_entity_t *entity);
    int     (*mod)          (z_iothread_t *iothread,
                             z_iopoll_entity_t *entity);
    int     (*remove)       (z_iothread_t *iothread,
                             z_iopoll_entity_t *entity);

    int     (*poll)         (z_iothread_t *iothread);
};

struct z_iopoll {
    Z_OBJECT_TYPE

    z_iopoll_plug_t *plug;
    int *            is_looping;

#ifdef Z_IOPOLL_USE_IOTHREAD
    z_iothread_t     iothreads[Z_IOPOLL_NTHREADS];
    z_spinlock_t     lock;
#else
    z_iothread_t     iothread;
#endif /* Z_IOPOLL_USE_IOTHREAD */

    z_hash_table_t   table;
    unsigned int     timeout;
    unsigned int     nthreads;
};

#ifdef Z_IOPOLL_HAS_EPOLL
    extern z_iopoll_plug_t z_iopoll_epoll;
#endif /* Z_IOPOLL_HAS_EPOLL */

#ifdef Z_IOPOLL_HAS_KQUEUE
    extern z_iopoll_plug_t z_iopoll_kqueue;
#endif /* Z_IOPOLL_HAS_KQUEUE */

extern z_iopoll_plug_t *z_iopoll_default;

z_iopoll_t *        z_iopoll_alloc      (z_iopoll_t *iopoll,
                                         z_memory_t *memory,
                                         z_iopoll_plug_t *plug,
                                         unsigned int timeout,
                                         int *is_looping);
void                z_iopoll_free       (z_iopoll_t *iopoll);

int                 z_iopoll_add        (z_iopoll_t *iopoll,
                                         z_iopoll_entity_t *entity);
int                 z_iopoll_remove     (z_iopoll_t *iopoll,
                                         z_iopoll_entity_t *entity);

int                 z_iopoll_suspend    (z_iopoll_t *iopoll,
                                         z_iopoll_entity_t *entity);
int                 z_iopoll_resume     (z_iopoll_t *iopoll,
                                         z_iopoll_entity_t *entity);

z_iopoll_entity_t * z_iopoll_lookup     (z_iopoll_t *iopoll,
                                         int fd);

int                 z_iopoll_loop       (z_iopoll_t *iopoll);

#define z_iopoll_is_looping(iopoll)                                          \
    (((iopoll)->is_looping != NULL) ? (*((iopoll)->is_looping)) : 1)

#define z_iopoll_entity_init(entity, efd, eplug)                             \
    do {                                                                     \
        Z_IOPOLL_ENTITY(entity)->fd = (efd);                                 \
        Z_IOPOLL_ENTITY(entity)->plug = (eplug);                             \
        Z_IOPOLL_ENTITY(entity)->flags = (3);                                \
    } while (0)

#define z_iopoll_entity_flag(entity, nflag)                                  \
    z_bit_test(Z_IOPOLL_ENTITY(entity)->flags, (nflag) + 2)

#define z_iopoll_entity_set_flag(entity, nflag)                              \
    z_bit_set(Z_IOPOLL_ENTITY(entity)->flags, (nflag) + 2)

#define z_iopoll_entity_clear_flag(entity, nflag)                            \
    z_bit_clear(Z_IOPOLL_ENTITY(entity)->flags, (nflag) + 2)

#define z_iopoll_entity_change_flag(entity, nflag, value)                    \
    z_bit_change(Z_IOPOLL_ENTITY(entity)->flags, (nflag) + 2, value)

#define z_iopoll_entity_reset_flags(entity)                                  \
    do { Z_IOPOLL_ENTITY(entity)->flags &= 3; } while (0)

#define z_iopoll_entity_is_readable(entity)                                  \
    z_bit_test(Z_IOPOLL_ENTITY(entity)->flags, 0)

#define z_iopoll_entity_is_writeable(entity)                                 \
    z_bit_test(Z_IOPOLL_ENTITY(entity)->flags, 1)

int     z_iopoll_entity_set_readable    (z_iopoll_t *iopoll,
                                         z_iopoll_entity_t *entity,
                                         int readable);
int     z_iopoll_entity_set_writeable   (z_iopoll_t *iopoll,
                                         z_iopoll_entity_t *entity,
                                         int writeable);

__Z_END_DECLS__

#endif /* !_Z_IOPOLL_H_ */

