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
void z_iopoll_stats_add_events (z_iopoll_t *iopoll, int nevents, uint64_t wait_time) {
    z_iopoll_stats_t *stats = &(iopoll->stats);
    z_histogram_add(&(stats->iowait), wait_time);
    if (nevents > stats->max_events)
        stats->max_events = nevents;
}

void z_iopoll_stats_add_read_event (z_iopoll_t *iopoll, uint64_t time) {
    z_histogram_add(&(iopoll->stats.ioread), time);
}

void z_iopoll_stats_add_write_event (z_iopoll_t *iopoll, uint64_t time) {
    z_histogram_add(&(iopoll->stats.iowrite), time);
}

void z_iopoll_stats_dump (z_iopoll_t *iopoll) {
    z_iopoll_stats_t *stats = &(iopoll->stats);
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
 *  PUBLIC I/O Poll Methods
 */
int z_iopoll_open (z_iopoll_t *iopoll,
                   z_memory_t *memory,
                   const z_vtable_iopoll_t *vtable)
{
    z_memzero(iopoll, sizeof(z_iopoll_t));
    if (vtable == NULL) {
#if defined(Z_IOPOLL_HAS_EPOLL)
        iopoll->vtable = &z_iopoll_epoll;
#elif defined(Z_IOPOLL_HAS_KQUEUE)
        iopoll->vtable = &z_iopoll_kqueue;
#endif
    } else {
        iopoll->vtable = vtable;
    }
    z_iopoll_stats_alloc(&(iopoll->stats), memory);
    return(iopoll->vtable->open(iopoll));
}

void z_iopoll_close (z_iopoll_t *iopoll) {
    iopoll->vtable->close(iopoll);
    z_iopoll_stats_free(&(iopoll->stats));
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

void z_iopoll_process (z_iopoll_t *iopoll,
                       z_iopoll_entity_t *entity,
                       uint32_t events)
{
    const z_vtable_iopoll_entity_t *vtable = entity->vtable;

    if (Z_UNLIKELY(events & Z_IOPOLL_HANGUP)) {
        z_iopoll_remove(iopoll, entity);
        vtable->close(iopoll, entity);
        return;
    }

    if (events & Z_IOPOLL_READABLE) {
        z_timer_t timer;
        z_timer_start(&timer);
        if (vtable->read(iopoll, entity) < 0) {
            z_iopoll_remove(iopoll, entity);
            vtable->close(iopoll, entity);
            return;
        }
        z_timer_stop(&timer);
        z_iopoll_stats_add_read_event(iopoll, z_timer_micros(&timer));
    }

    if (events & Z_IOPOLL_WRITABLE) {
        z_timer_t timer;
        z_timer_start(&timer);
        if (vtable->write(iopoll, entity) < 0) {
            z_iopoll_remove(iopoll, entity);
            vtable->close(iopoll, entity);
            return;
        }
        z_timer_stop(&timer);
        z_iopoll_stats_add_write_event(iopoll, z_timer_micros(&timer));
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
        entity->flags ^= Z_IOPOLL_WRITABLE;
        z_iopoll_add(iopoll, entity);
    }
}
