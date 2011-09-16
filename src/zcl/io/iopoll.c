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

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include <zcl/system.h>
#include <zcl/iopoll.h>

#if defined(Z_IOPOLL_HAS_EPOLL)
    z_iopoll_plug_t *z_iopoll_default = &z_iopoll_epoll;
#elif defined(Z_IOPOLL_HAS_KQUEUE)
    z_iopoll_plug_t *z_iopoll_default = &z_iopoll_kqueue;
#else
    z_iopoll_plug_t *z_iopoll_default = NULL;
#endif

#ifdef Z_IOPOLL_USE_IOTHREAD
    #define __IOPOLL_LOCK_INIT(iopoll)         z_spin_init(&((iopoll)->lock))
    #define __IOPOLL_LOCK_DESTROY(iopoll)      z_spin_destroy(&((iopoll)->lock))
    #define __IOPOLL_LOCK(iopoll)              z_spin_lock(&((iopoll)->lock))
    #define __IOPOLL_UNLOCK(iopoll)            z_spin_unlock(&((iopoll)->lock))

    #define __entity_thread(iopoll, entity)    ((entity)->iothread)
#else
    #define __IOPOLL_LOCK(iopoll)              while (0)
    #define __IOPOLL_UNLOCK(iopoll)            while (0)

    #define __entity_thread(iopoll, entity)    (&((iopoll)->iothread))
#endif

#ifdef Z_IOPOLL_USE_IOTHREAD
static z_iothread_t *__iothread_balance (z_iopoll_t *iopoll) {
    z_iothread_t *iothread;
    z_iothread_t *p;
    unsigned int i;

    iothread = &(iopoll->iothreads[0]);
    for (i = 1; i < iopoll->nthreads; ++i) {
        p = &(iopoll->iothreads[i]);
        if (p->nfds < iothread->nfds)
            iothread = p;
    }

    return(iothread);
}

static void *__iothread_poll (void *arg) {
    Z_IOTHREAD(arg)->iopoll->plug->poll(Z_IOTHREAD(arg));
    return(NULL);
}
#endif /* Z_IOPOLL_USE_IOTHREAD */

static unsigned int __iopoll_fd_hash (void *data, const void *key) {
    return(Z_UINT_PTR_VALUE(key));
}

static int __iopoll_fd_compare (void *data, const void *a, const void *b) {
    return(Z_IOPOLL_ENTITY(a)->fd - Z_INT_PTR_VALUE(b));
}

static unsigned int __iopoll_entity_hash (void *data, const void *entity) {
    return(Z_IOPOLL_ENTITY(entity)->fd);
}

static int __iopoll_entity_compare (void *data, const void *a, const void *b) {
    return(Z_IOPOLL_ENTITY(a)->fd - Z_IOPOLL_ENTITY(b)->fd);
}

static void __iopoll_entity_free (void *data, void *entity) {
    z_iopoll_entity_plug_t *plug = Z_IOPOLL_ENTITY(entity)->plug;
    if (plug->free != NULL)
        plug->free(data, Z_IOPOLL_ENTITY(entity));
}

z_iopoll_t *z_iopoll_alloc (z_iopoll_t *iopoll,
                            z_memory_t *memory,
                            z_iopoll_plug_t *plug,
                            unsigned int timeout,
                            int *is_looping)
{
    z_iothread_t *iothread;
#ifdef Z_IOPOLL_USE_IOTHREAD
    unsigned int i;
#endif

    if ((iopoll = z_object_alloc(memory, iopoll, z_iopoll_t)) == NULL)
        return(NULL);

    iopoll->plug = (plug != NULL) ? plug : z_iopoll_default;
    iopoll->timeout = timeout;
    iopoll->is_looping = is_looping;

    if (z_hash_table_alloc(&(iopoll->table), memory,
                           &z_hash_table_chain,
                           128,
                           __iopoll_entity_hash,
                           __iopoll_entity_compare,
                           z_hash_table_grow_policy,
                           z_hash_table_shrink_policy,
                           __iopoll_entity_free,
                           iopoll) == NULL)
    {
        z_object_free(iopoll);
        return(NULL);
    }

#ifdef Z_IOPOLL_USE_IOTHREAD
    if (__IOPOLL_LOCK_INIT(iopoll)) {
        z_hash_table_free(&(iopoll->table));
        z_object_free(iopoll);
        return(NULL);
    }

    iopoll->nthreads = z_min(z_system_processors(), Z_IOPOLL_NTHREADS);
    for (i = 0; i < iopoll->nthreads; ++i) {
        iothread = &(iopoll->iothreads[i]);
        iothread->iopoll = iopoll;
        iothread->nfds = 0;

        if (iopoll->plug->init(iothread)) {
            while (i--)
                iopoll->plug->uninit(&(iopoll->iothreads[i]));

            z_hash_table_free(&(iopoll->table));
            __IOPOLL_LOCK_DESTROY(iopoll);
            z_object_free(iopoll);
            return(NULL);
        }
    }
#else
    iothread = &(iopoll->iothread);
    iothread->iopoll = iopoll;

    if (iopoll->plug->init(iothread)) {
        z_hash_table_free(&(iopoll->table));
        z_object_free(iopoll);
        return(NULL);
    }
#endif

    return(iopoll);
}

void z_iopoll_free (z_iopoll_t *iopoll) {
#ifdef Z_IOPOLL_USE_IOTHREAD
    unsigned int i;

    for (i = 0; i < iopoll->nthreads; ++i)
        iopoll->plug->uninit(&(iopoll->iothreads[i]));

    __IOPOLL_LOCK_DESTROY(iopoll);
#else
    iopoll->plug->uninit(&(iopoll->iothread));
#endif

    z_hash_table_free(&(iopoll->table));
    z_object_free(iopoll);
}

int z_iopoll_add (z_iopoll_t *iopoll, z_iopoll_entity_t *entity) {
    __IOPOLL_LOCK(iopoll);

    if (z_hash_table_insert(&(iopoll->table), entity)) {
        __IOPOLL_UNLOCK(iopoll);
        return(1);
    }

#ifdef Z_IOPOLL_USE_IOTHREAD
    entity->iothread = __iothread_balance(iopoll);
    entity->iothread->nfds++;
#endif

    if (iopoll->plug->add(__entity_thread(iopoll, entity), entity)) {
        z_hash_table_remove(&(iopoll->table), entity);
        __IOPOLL_UNLOCK(iopoll);
        return(2);
    }

    __IOPOLL_UNLOCK(iopoll);
    return(0);
}

int z_iopoll_remove (z_iopoll_t *iopoll, z_iopoll_entity_t *entity) {
    z_iothread_t *iothread = __entity_thread(iopoll, entity);

    __IOPOLL_LOCK(iopoll);

    if (!iopoll->plug->remove(iothread, entity))
        iothread--;
    z_hash_table_remove(&(iopoll->table), entity);

    __IOPOLL_UNLOCK(iopoll);
    return(0);
}

int z_iopoll_suspend (z_iopoll_t *iopoll, z_iopoll_entity_t *entity) {
    int ret;

    __IOPOLL_LOCK(iopoll);
    ret = !!(iopoll->plug->remove(__entity_thread(iopoll, entity), entity));
    __IOPOLL_UNLOCK(iopoll);

    return(ret);
}

int z_iopoll_resume (z_iopoll_t *iopoll, z_iopoll_entity_t *entity) {
    int ret;

    __IOPOLL_LOCK(iopoll);
    ret = !!(iopoll->plug->add(__entity_thread(iopoll, entity), entity));
    __IOPOLL_UNLOCK(iopoll);

    return(ret);
}

z_iopoll_entity_t *z_iopoll_lookup (z_iopoll_t *iopoll, int fd) {
    void *entity;

    __IOPOLL_LOCK(iopoll);
    entity = z_hash_table_lookup_custom(&(iopoll->table),
                                        __iopoll_fd_hash,
                                        __iopoll_fd_compare, &fd);
    __IOPOLL_UNLOCK(iopoll);

    return(Z_IOPOLL_ENTITY(entity));
}

int z_iopoll_loop (z_iopoll_t *iopoll) {
#ifdef Z_IOPOLL_USE_IOTHREAD
    unsigned int i;

    for (i = 1; i < iopoll->nthreads; ++i) {
        z_thread_create(&(iopoll->iothreads[i].thread),
                        __iothread_poll,
                        &(iopoll->iothreads[i]));
    }

    iopoll->plug->poll(&(iopoll->iothreads[0]));
    for (i = 1; i < iopoll->nthreads; ++i)
        z_thread_join(&(iopoll->iothreads[i].thread));
#else
    iopoll->plug->poll(&(iopoll->iothread));
#endif

    return(0);
}

int z_iopoll_entity_set_readable (z_iopoll_t *iopoll,
                                  z_iopoll_entity_t *entity,
                                  int readable)
{
    if (z_bit_test(&(entity->flags), 0) == readable)
        return(0);

    z_bit_change(&(entity->flags), 0, readable);
    return(iopoll->plug->mod(__entity_thread(iopoll, entity), entity));
}

int z_iopoll_entity_set_writeable (z_iopoll_t *iopoll,
                                   z_iopoll_entity_t *entity,
                                   int writeable)
{
    if (z_bit_test(&(entity->flags), 1) == writeable)
        return(0);

    z_bit_change(&(entity->flags), 1, writeable);
    return(iopoll->plug->mod(__entity_thread(iopoll, entity), entity));
}

