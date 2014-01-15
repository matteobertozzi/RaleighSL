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

#include <unistd.h>

#include <zcl/system.h>
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

#define __STATS_HISTO_NBOUNDS  (sizeof(__STATS_HISTO_BOUNDS) / sizeof(uint64_t))

/* ===========================================================================
 *  PRIVATE I/O Poll Stats Methods
 */
static int z_iopoll_stats_alloc (z_iopoll_stats_t *stats) {
  z_histogram_open(&(stats->iowait),  __STATS_HISTO_BOUNDS, stats->iowait_events,  __STATS_HISTO_NBOUNDS);
  z_histogram_open(&(stats->ioread),  __STATS_HISTO_BOUNDS, stats->ioread_events,  __STATS_HISTO_NBOUNDS);
  z_histogram_open(&(stats->iowrite), __STATS_HISTO_BOUNDS, stats->iowrite_events, __STATS_HISTO_NBOUNDS);
  stats->max_events = 0;
  return(0);
}

static void z_iopoll_stats_free (z_iopoll_stats_t *stats) {
  z_histogram_close(&(stats->iowait));
  z_histogram_close(&(stats->ioread));
  z_histogram_close(&(stats->iowrite));
}

/* ===========================================================================
 *  PUBLIC I/O Poll Stats Methods
 */
void z_iopoll_stats_add_events (z_iopoll_engine_t *engine, int nevents, uint64_t wait_time) {
  z_iopoll_stats_t *stats = &(engine->stats);
  z_histogram_add(&(stats->iowait), wait_time);
  stats->max_events = z_max(stats->max_events, nevents);
}

void z_iopoll_stats_add_read_event (z_iopoll_engine_t *engine, uint64_t rtime) {
  z_histogram_add(&(engine->stats.ioread), rtime);
}

void z_iopoll_stats_add_write_event (z_iopoll_engine_t *engine, uint64_t wtime) {
  z_histogram_add(&(engine->stats.iowrite), wtime);
}

void z_iopoll_stats_dump (z_iopoll_engine_t *engine) {
  z_iopoll_stats_t *stats = &(engine->stats);
  char buf0[16], buf1[16], buf2[16];

  printf("IOPoll Stats\n");
  printf("==================================\n");
  printf("max   events:   %"PRIu32"\n", stats->max_events);
  printf("poll  swtich:   %"PRIu64"\n", stats->iowait.nevents);
  printf("read  events:   %"PRIu64"\n", stats->ioread.nevents);
  printf("write events:   %"PRIu64"\n", stats->iowrite.nevents);
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

  fprintf(stdout, "IOWait ");
  z_histogram_dump(&(stats->iowait), stdout, z_human_time);

  fprintf(stdout, "IORead ");
  z_histogram_dump(&(stats->ioread), stdout, z_human_time);

  fprintf(stdout, "IOWrite ");
  z_histogram_dump(&(stats->iowrite), stdout, z_human_time);
}

/* ===========================================================================
 *  PRIVATE I/O Poll Engine Methods
 */
#if Z_IOPOLL_ENGINES > 1
  #define __iopoll_nengines(iopoll)          ((iopoll)->nengines)
#else
  #define __iopoll_nengines(iopoll)          (1)
#endif

#define __iopoll_engine_slot(iopoll)         ((iopoll)->balancer++ % __iopoll_nengines(iopoll))

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

static z_iopoll_engine_t *__iopoll_get_engine (z_iopoll_t *iopoll) {
  z_iopoll_engine_t *engine;
  unsigned int count;
  z_thread_t tid;

  z_thread_self(&tid);
  count = __iopoll_nengines(iopoll);
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

static int __iopoll_engine_open (z_iopoll_t *iopoll, z_iopoll_engine_t *engine)
{
  if (z_iopoll_stats_alloc(&(engine->stats)))
    return(1);

  if (iopoll->vtable->open(iopoll, engine)) {
    z_iopoll_stats_free(&(engine->stats));
    return(2);
  }

  return(0);
}

/* ===========================================================================
 *  PRIVATE I/O Poll Threads Methods
 */
static void __iopoll_engine_close (z_iopoll_t *iopoll, z_iopoll_engine_t *engine) {
  iopoll->vtable->close(iopoll, engine);
  z_iopoll_stats_free(&(engine->stats));
}

static int __iopoll_threads_start (z_iopoll_t *iopoll, unsigned int i) {
  for (; i < __iopoll_nengines(iopoll)  ; ++i) {
    z_iopoll_engine_t *engine = &(iopoll->engines[i]);
    if (z_thread_start(&(engine->thread), __iopoll_engine_do_poll, iopoll)) {
      iopoll->is_looping = NULL;
      return(1);
    }

    z_thread_bind_to_core(&(engine->thread), i);
  }
  return(0);
}

static void __iopoll_threads_wait (z_iopoll_t *iopoll, unsigned int i) {
  for (; i < __iopoll_nengines(iopoll)  ; ++i) {
    z_thread_join(&(iopoll->engines[i].thread));
  }
}

/* ===========================================================================
 *  PUBLIC I/O Poll Methods
 */
int z_iopoll_open (z_iopoll_t *iopoll,
                   const z_vtable_iopoll_t *vtable,
                   unsigned int nengines)
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

#if (Z_IOPOLL_ENGINES > 1)
  /* If the number of engine is not specified use all the cores */
  if (nengines == 0) {
    nengines = z_system_processors();
    Z_LOG_DEBUG("Use syste proc %u engines for IOPoll", nengines);
  }

  Z_LOG_DEBUG("Use %u engines for IOPoll", nengines);
  iopoll->nengines = z_min(nengines, Z_IOPOLL_ENGINES);
  iopoll->balancer = 0;
  Z_LOG_DEBUG("Use %u engines for IOPoll", iopoll->nengines);
#endif /* Z_IOPOLL_ENGINES > 1 */

  for (i = 0; i < __iopoll_nengines(iopoll)  ; ++i) {
    if (__iopoll_engine_open(iopoll, &(iopoll->engines[i]))) {
      while (i-- > 0) {
        __iopoll_engine_close(iopoll, &(iopoll->engines[i]));
      }
      return(1);
    }
  }
  return(0);
}

void z_iopoll_close (z_iopoll_t *iopoll) {
  unsigned int i;
  for (i = 0; i < __iopoll_nengines(iopoll)  ; ++i) {
    __iopoll_engine_close(iopoll, &(iopoll->engines[i]));
  }
}

int z_iopoll_add (z_iopoll_t *iopoll, z_iopoll_entity_t *entity) {
#if (Z_IOPOLL_ENGINES > 1)
  unsigned int eidx = __iopoll_engine_slot(iopoll);
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

int z_iopoll_poll (z_iopoll_t *iopoll, int detached, const int *is_looping, int timeout) {
  iopoll->is_looping = is_looping;
  iopoll->timeout = timeout;

  if (detached) {
    __iopoll_threads_start(iopoll, 0);
  } else {
    /* engine[1:] are running on other threads */
    #if (Z_IOPOLL_ENGINES > 1)
      __iopoll_threads_start(iopoll, 1);
    #endif /* Z_IOPOLL_ENGINES > 1 */

    /* engine[0] is running on the main thread */
    __iopoll_engine_poll(iopoll, &(iopoll->engines[0]));

    #if (Z_IOPOLL_ENGINES > 1)
      __iopoll_threads_wait(iopoll, 1);
    #endif /* Z_IOPOLL_ENGINES > 1 */
  }

  return(0);
}

void z_iopoll_wait (z_iopoll_t *iopoll) {
  __iopoll_threads_wait(iopoll, 0);
}

void z_iopoll_process (z_iopoll_t *iopoll,
                       z_iopoll_engine_t *engine,
                       z_iopoll_entity_t *entity,
                       uint32_t events)
{
  const z_vtable_iopoll_entity_t *vtable = entity->vtable;
  uint64_t tnow;

  if (Z_UNLIKELY(events & Z_IOPOLL_HANGUP)) {
    __iopoll_engine_remove(iopoll, engine, entity);
    vtable->close(entity);
    return;
  }

  tnow = z_time_micros();
  if (events & Z_IOPOLL_READABLE) {
    uint64_t rstime = tnow;
    if (Z_UNLIKELY(vtable->read(entity) < 0)) {
      __iopoll_engine_remove(iopoll, engine, entity);
      vtable->close(entity);
      return;
    }
    tnow = z_time_micros();
    z_iopoll_stats_add_read_event(engine, tnow - rstime);
  }

  if (events & Z_IOPOLL_WRITABLE) {
    if (entity->flags & Z_IOPOLL_HAS_DATA) {
      uint64_t wstime = tnow;
      if (Z_UNLIKELY(vtable->write(entity) < 0)) {
        __iopoll_engine_remove(iopoll, engine, entity);
        vtable->close(entity);
        return;
      }
      tnow = z_time_micros();
      z_iopoll_stats_add_write_event(engine, tnow - wstime);
    } else if ((tnow - entity->last_wavail) > 1000000) {
      /* Apply the changes after 1sec of no data, to avoid too may context switch */
      z_spin_lock(&(entity->lock));
      if (!(entity->flags & Z_IOPOLL_HAS_DATA)) {
        entity->flags ^= Z_IOPOLL_WRITABLE;
        __iopoll_engine_insert(iopoll, engine, entity);
      }
      z_spin_unlock(&(entity->lock));
    }
  }
}

void z_iopoll_entity_open (z_iopoll_entity_t *entity,
                           const z_vtable_iopoll_entity_t *vtable,
                           int fd)
{
  entity->vtable = vtable;
  entity->flags = Z_IOPOLL_READABLE;
  entity->fd = fd;
  z_spin_alloc(&(entity->lock));
}

void z_iopoll_entity_close (z_iopoll_entity_t *entity) {
  z_spin_free(&(entity->lock));
  close(entity->fd);
}

void z_iopoll_set_writable (z_iopoll_t *iopoll,
                            z_iopoll_entity_t *entity,
                            int writable)
{
  z_spin_lock(&(entity->lock));
  if (writable != !!(entity->flags & Z_IOPOLL_HAS_DATA)) {
    entity->flags ^= Z_IOPOLL_HAS_DATA;
    if (writable && !(entity->flags & Z_IOPOLL_WRITABLE)) {
      z_iopoll_engine_t *engine = __iopoll_entity_engine(iopoll, entity);
      entity->flags ^= Z_IOPOLL_WRITABLE;
      __iopoll_engine_insert(iopoll, engine, entity);
    }
  }
  entity->last_wavail = z_time_micros();
  z_spin_unlock(&(entity->lock));
}
