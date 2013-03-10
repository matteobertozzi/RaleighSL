/*
 *   Copyright 2011-2013 Matteo Bertozzi
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

#include <zcl/humans.h>
#include <zcl/string.h>
#include <zcl/iopoll.h>
#include <zcl/time.h>

/* ===========================================================================
 *  PRIVATE I/O Poll Stats
 */
static const uint64_t __STATS_HISTO_BOUNDS[] = {
  Z_TIME_USEC(5),  Z_TIME_USEC(10),  Z_TIME_USEC(20),  Z_TIME_USEC(50),
  Z_TIME_MSEC(1),  Z_TIME_MSEC(5),   Z_TIME_MSEC(10),  Z_TIME_MSEC(25),
  Z_TIME_MSEC(50), Z_TIME_MSEC(100), Z_TIME_MSEC(250), Z_TIME_MSEC(500),
  Z_TIME_SEC(1),   Z_TIME_SEC(2),    Z_TIME_SEC(5),    Z_TIME_SEC(10)
};

/* ===========================================================================
 *  PRIVATE I/O Poll Stats Methods
 */
static void z_iopoll_stats_alloc (z_iopoll_stats_t *stats, z_memory_t *memory) {
    unsigned int nbounds = sizeof(__STATS_HISTO_BOUNDS) / sizeof(uint64_t);
    z_histogram_alloc(&(stats->iowait), memory, __STATS_HISTO_BOUNDS, nbounds);
    z_histogram_alloc(&(stats->ioread), memory, __STATS_HISTO_BOUNDS, nbounds);
    z_histogram_alloc(&(stats->iowrite), memory, __STATS_HISTO_BOUNDS, nbounds);
    stats->max_events = 0;
}

static void z_iopoll_stats_free (z_iopoll_stats_t *stats) {
    z_histogram_free(&(stats->iowait));
    z_histogram_free(&(stats->ioread));
    z_histogram_free(&(stats->iowrite));
}

/* ===========================================================================
 *  PUBLIC I/O Poll Stats Methods
 */
void z_iopoll_stats_add_events (z_iopoll_engine_t *engine, int nevents, uint64_t wait_time) {
    z_iopoll_stats_t *stats = &(engine->stats);
    z_histogram_add(&(stats->iowait), wait_time);
    if (nevents > stats->max_events)
        stats->max_events = nevents;
}

void z_iopoll_stats_add_read_event (z_iopoll_engine_t *engine, uint64_t time) {
    z_histogram_add(&(engine->stats.ioread), time);
}

void z_iopoll_stats_add_write_event (z_iopoll_engine_t *engine, uint64_t time) {
    z_histogram_add(&(engine->stats.iowrite), time);
}

void z_iopoll_stats_dump (z_iopoll_engine_t *engine) {
    z_iopoll_stats_t *stats = &(engine->stats);
    char buf0[16], buf1[16], buf2[16];

    printf("IOPoll Stats\n");
    printf("==================================\n");
    printf("max events:     %u\n", stats->max_events);
    printf("poll swtich:    %lu\n", stats->iowait.nevents);
    printf("read events:    %lu\n", stats->ioread.nevents);
    printf("write events:   %lu\n", stats->iowrite.nevents);
    printf("avg IO wait:    %s (%s-%s)\n",
          z_human_time(buf0, sizeof(buf0), z_histogram_average(&(stats->iowait))),
          z_human_time(buf1, sizeof(buf1), z_histogram_percentile(&(stats->iowait), 0)),
          z_human_time(buf2, sizeof(buf2), z_histogram_percentile(&(stats->iowait), 99.9999)));
    printf("avg IO read:    %s (%s-%s)\n",
          z_human_time(buf0, sizeof(buf0), z_histogram_average(&(stats->ioread))),
          z_human_time(buf1, sizeof(buf1), z_histogram_percentile(&(stats->ioread), 0)),
          z_human_time(buf2, sizeof(buf2), z_histogram_percentile(&(stats->ioread), 99.9999)));
    printf("avg IO write:   %s (%s-%s)\n",
          z_human_time(buf0, sizeof(buf0), z_histogram_average(&(stats->iowrite))),
          z_human_time(buf1, sizeof(buf1), z_histogram_percentile(&(stats->iowrite), 0)),
          z_human_time(buf2, sizeof(buf2), z_histogram_percentile(&(stats->iowrite), 99.9999)));

    unsigned int i, nbounds = sizeof(__STATS_HISTO_BOUNDS) / sizeof(uint64_t);

    printf("IOWait (MIN %lu MAX %lu)\n", stats->iowait.min, stats->iowait.max);
    for (i = 0; i < nbounds; ++i)
      printf("%14s %lu\n", z_human_time(buf0, sizeof(buf0), stats->iowait.bounds[i]), stats->iowait.events[i]);
    printf("     %lu\n", stats->iowait.events[i]);

    printf("IORead (MIN %lu MAX %lu)\n", stats->ioread.min, stats->ioread.max);
    for (i = 0; i < nbounds; ++i)
      printf("%14s %lu\n", z_human_time(buf0, sizeof(buf0), stats->ioread.bounds[i]), stats->ioread.events[i]);
    printf("     %lu\n", stats->ioread.events[i]);

    printf("IOWrite (MIN %lu MAX %lu)\n", stats->iowrite.min, stats->iowrite.max);
    for (i = 0; i < nbounds; ++i)
      printf("%14s %lu\n", z_human_time(buf0, sizeof(buf0), stats->iowrite.bounds[i]), stats->iowrite.events[i]);
    printf("     %lu\n", stats->iowrite.events[i]);
}

/* ===========================================================================
 *  PRIVATE I/O Poll Engine Methods
 */
#define __iopoll_entity_engine(iopoll, entity)                              \
    &((iopoll)->engines[(entity)->flags >> 28])

#define __iopoll_entity_set_engine_id(iopoll, entity, engine)               \
    ((entity)->flags = ((entity)->flags & 0xfffffff) | (engine << 28))

#define __iopoll_engine_insert(iopoll, engine, entity)                      \
    (iopoll)->vtable->insert(iopoll, engine, entity)

#define __iopoll_engine_remove(iopoll, engine, entity)                      \
    (iopoll)->vtable->remove(iopoll, engine, entity)

#define __iopoll_engine_poll(iopoll, engine)                                \
    (iopoll)->vtable->poll(iopoll, engine)

#if (Z_IOPOLL_ENGINES > 1)
static z_iopoll_engine_t *__iopoll_get_engine (z_iopoll_t *iopoll) {
    z_iopoll_engine_t *engine;
    unsigned int count;
    z_thread_t tid;

    z_thread_self(&tid);
    count = Z_IOPOLL_ENGINES;
    engine = iopoll->engines;
    while (count--) {
        if (engine->thread == tid)
            return(engine);
        engine++;
    }

    return(NULL);
}

static void *__iopoll_engine_do_poll (void *args) {
    __iopoll_engine_poll(Z_IOPOLL(args), __iopoll_get_engine(Z_IOPOLL(args)));
    return(NULL);
}
#endif /* Z_IOPOLL_ENGINES > 1 */

static int __iopoll_engine_open (z_iopoll_t *iopoll,
                                 z_iopoll_engine_t *engine,
                                 z_memory_t *memory)
{
    z_iopoll_stats_alloc(&(engine->stats), memory);
    return(iopoll->vtable->open(iopoll, engine));
}

static void __iopoll_engine_close (z_iopoll_t *iopoll, z_iopoll_engine_t *engine) {
    iopoll->vtable->close(iopoll, engine);
    z_iopoll_stats_free(&(engine->stats));
}

/* ===========================================================================
 *  PUBLIC I/O Poll Methods
 */
int z_iopoll_open (z_iopoll_t *iopoll,
                   z_memory_t *memory,
                   const z_vtable_iopoll_t *vtable)
{
    unsigned int i;

    if (vtable == NULL) {
#if defined(Z_IOPOLL_HAS_EPOLL)
        iopoll->vtable = &z_iopoll_epoll;
#elif defined(Z_IOPOLL_HAS_KQUEUE)
        iopoll->vtable = &z_iopoll_kqueue;
#endif
    } else {
        iopoll->vtable = vtable;
    }

    for (i = 0; i < Z_IOPOLL_ENGINES; ++i) {
        if (__iopoll_engine_open(iopoll, &(iopoll->engines[i]), memory)) {
            while (i-- > 0) {
                __iopoll_engine_close(iopoll, &(iopoll->engines[i]));
            }
            return(1);
        }
    }

#if (Z_IOPOLL_ENGINES > 1)
    iopoll->balancer = 0;
#endif /* Z_IOPOLL_ENGINES > 1 */

    return(0);
}

void z_iopoll_close (z_iopoll_t *iopoll) {
    unsigned int i;
    for (i = 0; i < Z_IOPOLL_ENGINES; ++i) {
        __iopoll_engine_close(iopoll, &(iopoll->engines[i]));
    }
}

int z_iopoll_add (z_iopoll_t *iopoll, z_iopoll_entity_t *entity) {
#if (Z_IOPOLL_ENGINES > 1)
    unsigned int eidx = iopoll->balancer++ & (Z_IOPOLL_ENGINES - 1);
#else
    unsigned int eidx = 0;
#endif
    __iopoll_entity_set_engine_id(iopoll, entity, eidx);
    return(__iopoll_engine_insert(iopoll, &(iopoll->engines[eidx]), entity));
}

int z_iopoll_remove (z_iopoll_t *iopoll, z_iopoll_entity_t *entity) {
    z_iopoll_engine_t *engine = __iopoll_entity_engine(iopoll, entity);
    return(__iopoll_engine_remove(iopoll, engine, entity));
}

int z_iopoll_poll (z_iopoll_t *iopoll, const int *is_looping, int timeout) {
#if (Z_IOPOLL_ENGINES > 1)
    unsigned int i;
#endif /* Z_IOPOLL_ENGINES > 1 */

    iopoll->is_looping = is_looping;
    iopoll->timeout = timeout;

#if (Z_IOPOLL_ENGINES > 1)
    for (i = 1; i < Z_IOPOLL_ENGINES; ++i) {
        z_iopoll_engine_t *engine = &(iopoll->engines[i]);
        if (z_thread_alloc(&(engine->thread), __iopoll_engine_do_poll, iopoll)) {
            iopoll->is_looping = NULL;
            return(1);
        }

        z_thread_bind_to_core(&(engine->thread), i - 1);
    }
#endif /* Z_IOPOLL_ENGINES > 1 */

    /* engine[0] is running on the main thread */
    __iopoll_engine_poll(iopoll, &(iopoll->engines[0]));
#if (Z_IOPOLL_ENGINES > 1)
    for (i = 1; i < Z_IOPOLL_ENGINES; ++i) {
        z_thread_join(&(iopoll->engines[i].thread));
    }
#endif /* Z_IOPOLL_ENGINES > 1 */

    return(0);
}

void z_iopoll_process (z_iopoll_t *iopoll,
                       z_iopoll_engine_t *engine,
                       z_iopoll_entity_t *entity,
                       uint32_t events)
{
    const z_vtable_iopoll_entity_t *vtable = entity->vtable;

    if (Z_UNLIKELY(events & Z_IOPOLL_HANGUP)) {
        __iopoll_engine_remove(iopoll, engine, entity);
        vtable->close(entity);
        return;
    }

    if (events & Z_IOPOLL_READABLE) {
        z_timer_t timer;
        z_timer_start(&timer);
        if (vtable->read(entity) < 0) {
            __iopoll_engine_remove(iopoll, engine, entity);
            vtable->close(entity);
            return;
        }
        z_timer_stop(&timer);
        z_iopoll_stats_add_read_event(engine, z_timer_micros(&timer));
    }

    if (events & Z_IOPOLL_WRITABLE) {
        z_timer_t timer;
        z_timer_start(&timer);
        if (vtable->write(entity) < 0) {
            __iopoll_engine_remove(iopoll, engine, entity);
            vtable->close(entity);
            return;
        }
        z_timer_stop(&timer);
        z_iopoll_stats_add_write_event(engine, z_timer_micros(&timer));
    }
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
        z_iopoll_engine_t *engine = __iopoll_entity_engine(iopoll, entity);
        entity->flags ^= Z_IOPOLL_WRITABLE;
        __iopoll_engine_insert(iopoll, engine, entity);
    }
}
