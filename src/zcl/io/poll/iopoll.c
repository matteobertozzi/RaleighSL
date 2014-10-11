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

#include "iopoll_private.h"
#include <zcl/threadpool.h>
#include <zcl/allocator.h>
#include <zcl/iopoll.h>
#include <zcl/debug.h>
#include <zcl/time.h>
#include <zcl/fd.h>

struct z_iopoll {
  const z_iopoll_vtable_t *vtable;
  const int *is_running;
  uint8_t pad1[Z_CACHELINE_PAD_FIELDS(z_iopoll_vtable_t *, int *)];

  z_thread_pool_t thread_pool;
  uint8_t pad2[Z_CACHELINE_PAD(sizeof(z_thread_pool_t))];

  z_iopoll_engine_t engines[1];
};

/* ============================================================================
 *  PRIVATE I/O Poll registration
 */
static const z_iopoll_entity_vtable_t *__iopoll_entity_vtables[] = {
  /*  0- 7 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  /*  8-15 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  /* 16-23 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  /* 24-31 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};

static uint8_t __iopoll_entity_register (const z_iopoll_entity_vtable_t *vtable) {
  uint8_t i;
  for (i = 0; i < z_fix_array_size(__iopoll_entity_vtables); ++i) {
    if (__iopoll_entity_vtables[i] == vtable || __iopoll_entity_vtables[i] == NULL) {
      __iopoll_entity_vtables[i] = vtable;
      return(i);
    }
  }
  Z_LOG_FATAL("Too many iopoll-entities registered: %d", i);
  return(0xff);
}

/* ============================================================================
 *  PRIVATE global-iopoll instance
 */
static z_iopoll_t *__global_iopoll = NULL;

#define __iopoll_engine_id(self)                                    \
  ((int)(Z_IOPOLL_ENGINE(self) - __global_iopoll->engines))

#define __iopoll_engine_add(engine, entity)                         \
  __global_iopoll->vtable->add(engine, entity)

#define __iopoll_engine_remove(engine, entity)                      \
  __global_iopoll->vtable->remove(engine, entity)

#define __iopoll_engine_timer(engine, entity, msec)                 \
  __global_iopoll->vtable->timer(engine, entity, msec)

#define __iopoll_engine_uevent(engine, entity, enable)              \
  __global_iopoll->vtable->uevent(engine, entity, enable)

#define __iopoll_engine_notify(engine, entity)                      \
  __global_iopoll->vtable->notify(engine, entity)

#define __iopoll_engine_poll(engine)                                \
  __global_iopoll->vtable->poll(engine)

/* ============================================================================
 *  INTERNAL methods called by the engines
 */
const int *z_iopoll_is_running (void) {
  return(__global_iopoll->is_running);
}

#define __write_time_elapsed(entity, now)                    \
  (((now) - (entity)->last_write_sec * 1000000u) > 0)

void z_iopoll_process (z_iopoll_engine_t *engine,
                       z_iopoll_entity_t *entity,
                       uint32_t events)
{
  const z_iopoll_entity_vtable_t *vtable = __iopoll_entity_vtables[entity->type];
  uint64_t stime, now;

  /* hangup event */
  if (Z_UNLIKELY(events & Z_IOPOLL_HANGUP)) {
    __iopoll_engine_remove(engine, entity);
    vtable->close(entity);
    return;
  }

  /* user event */
  if (Z_UNLIKELY(events & Z_IOPOLL_UEVENT)) {
    if (Z_UNLIKELY(vtable->uevent(entity) < 0)) {
      __iopoll_engine_remove(engine, entity);
      vtable->close(entity);
      return;
    }
  }

  /* timeout event */
  if (Z_UNLIKELY(events & Z_IOPOLL_TIMEOUT)) {
    if (Z_UNLIKELY(vtable->timeout(entity) < 0)) {
      __iopoll_engine_remove(engine, entity);
      vtable->close(entity);
      return;
    }
  }

  /* read event */
  stime = now = z_time_micros();
  if (events & Z_IOPOLL_READABLE) {
    if (Z_UNLIKELY(vtable->read(entity) < 0)) {
      __iopoll_engine_remove(engine, entity);
      vtable->close(entity);
      return;
    }
    now = z_time_micros();
    z_histogram_add(&(engine->stats.ioread), now - stime);
  }

  /* write event */
  if (events & Z_IOPOLL_WRITABLE || entity->iflags8 & Z_IOPOLL_HAS_DATA) {
    if (entity->iflags8 & Z_IOPOLL_HAS_DATA) {
      stime = now;
      if (Z_UNLIKELY(vtable->write(entity) < 0)) {
        __iopoll_engine_remove(engine, entity);
        vtable->close(entity);
        return;
      }
      now = z_time_micros();
      z_histogram_add(&(engine->stats.iowrite), now - stime);
    } else if (Z_UNLIKELY(events & Z_IOPOLL_WRITABLE && __write_time_elapsed(entity, now))) {
      /* Apply the changes after 1sec of no data, to avoid too may context switch */
      z_ticket_acquire(&(entity->lock));
      if (!(entity->iflags8 & Z_IOPOLL_HAS_DATA)) {
        entity->iflags8 ^= Z_IOPOLL_WRITABLE;
        __iopoll_engine_add(engine, entity);
      }
      z_ticket_release(&(entity->lock));
    }
  }
}

/* ===========================================================================
 *  PRIVATE I/O Poll Stats
 */
#define __STATS_HISTO_NBOUNDS       z_fix_array_size(__STATS_HISTO_BOUNDS)
static const uint64_t __STATS_HISTO_BOUNDS[24] = {
  Z_TIME_USEC(5),   Z_TIME_USEC(10),  Z_TIME_USEC(25),  Z_TIME_USEC(60),
  Z_TIME_USEC(80),  Z_TIME_USEC(125), Z_TIME_USEC(200), Z_TIME_USEC(350),
  Z_TIME_USEC(300), Z_TIME_USEC(350), Z_TIME_USEC(400), Z_TIME_USEC(450),
  Z_TIME_MSEC(500), Z_TIME_USEC(750), Z_TIME_MSEC(1),   Z_TIME_MSEC(5),
  Z_TIME_MSEC(10),  Z_TIME_MSEC(25),  Z_TIME_MSEC(75),  Z_TIME_MSEC(250),
  Z_TIME_MSEC(500), Z_TIME_MSEC(750), Z_TIME_SEC(1),    0xffffffffffffffffll,
};

/* ===========================================================================
 *  PRIVATE I/O Poll Threads Methods
 */
static void *__iopoll_engine_do_poll (void *arg) {
  __global_iopoll->vtable->poll(Z_IOPOLL_ENGINE(arg));
  z_thread_pool_worker_close(&(__global_iopoll->thread_pool));
  return(NULL);
}

static int __iopoll_threads_start (unsigned int index) {
  for (; index < __global_iopoll->thread_pool.size; ++index) {
    z_iopoll_engine_t *engine = &(__global_iopoll->engines[index]);
    if (z_thread_start(&(engine->thread), __iopoll_engine_do_poll, engine)) {
      __global_iopoll->is_running = NULL;
      return(1);
    }

    z_thread_bind_to_core(&(engine->thread), index);
  }
  return(0);
}

static void __iopoll_threads_wait (unsigned int index) {
  for (; index < __global_iopoll->thread_pool.size; ++index) {
    z_iopoll_engine_t *engine = &(__global_iopoll->engines[index]);
    z_thread_join(&(engine->thread));
  }
}

/* ===========================================================================
 *  PRIVATE I/O Poll engine methods
 */
static int __iopoll_engine_open (z_iopoll_engine_t *engine) {
  z_iopoll_stats_t *stats = &(engine->stats);

  /* Initialize I/O Poll Histogram */
  z_histogram_init(&(stats->iowait),  __STATS_HISTO_BOUNDS,
                   stats->iowait_events,  __STATS_HISTO_NBOUNDS);
  z_histogram_init(&(stats->ioread),  __STATS_HISTO_BOUNDS,
                   stats->ioread_events,  __STATS_HISTO_NBOUNDS);
  z_histogram_init(&(stats->iowrite), __STATS_HISTO_BOUNDS,
                   stats->iowrite_events, __STATS_HISTO_NBOUNDS);
  z_histogram_init(&(stats->req_latency), __STATS_HISTO_BOUNDS,
                   stats->req_latency_events, __STATS_HISTO_NBOUNDS);
  z_histogram_init(&(stats->resp_latency), __STATS_HISTO_BOUNDS,
                   stats->resp_latency_events, __STATS_HISTO_NBOUNDS);

  if (__global_iopoll->vtable->open(engine)) {
    return(1);
  }
  return(0);
}

static void __iopoll_engine_close (z_iopoll_engine_t *engine) {
  __global_iopoll->vtable->close(engine);
}

static void __iopoll_engine_dump (z_iopoll_engine_t *engine) {
  z_iopoll_stats_t *stats = &(engine->stats);

  printf("IOPoll Stats - ENGINE %d\n", __iopoll_engine_id(engine));
  printf("==================================\n");
  fprintf(stdout, "IOWait (Max Events %u) ", stats->iowait.max_events);
  z_histogram_dump(&(stats->iowait), stdout, z_human_dtime);
  fprintf(stdout, "\n");

  fprintf(stdout, "IORead ");
  z_histogram_dump(&(stats->ioread), stdout, z_human_dtime);
  fprintf(stdout, "\n");

  fprintf(stdout, "IOWrite ");
  z_histogram_dump(&(stats->iowrite), stdout, z_human_dtime);
  fprintf(stdout, "\n");

  fprintf(stdout, "Req Latency ");
  z_histogram_dump(&(stats->req_latency), stdout, z_human_dtime);
  fprintf(stdout, "\n");

  fprintf(stdout, "Resp Latency ");
  z_histogram_dump(&(stats->resp_latency), stdout, z_human_dtime);
  fprintf(stdout, "\n");
}

/* ===========================================================================
 *  PUBLIC I/O Poll init methods
 */
int z_iopoll_open (unsigned int ncores) {
  z_iopoll_engine_t *ioengine;
  size_t mmsize;

  if (__global_iopoll != NULL) {
    Z_LOG_WARN("iopoll exists already!");
    return(0);
  }

  /* Allocate the iopoll object */
  ncores = (ncores > 0 ? ncores : z_system_processors());
  mmsize = sizeof(z_iopoll_t) + ((ncores - 1) * sizeof(z_iopoll_engine_t));
  __global_iopoll = z_allocator_alloc(z_system_allocator(), z_iopoll_t, mmsize);
  if (__global_iopoll == NULL) {
    Z_LOG_ERROR("unable to allocate the iopoll context.");
    return(1);
  }

  Z_LOG_DEBUG("Initialize iopoll thread-pool size %u", ncores);
  if (z_thread_pool_open(&(__global_iopoll->thread_pool), ncores)) {
    Z_LOG_FATAL("unable to initialize the iopoll thread-pool.");
    z_allocator_free(z_system_allocator(), __global_iopoll);
    __global_iopoll = NULL;
    return(2);
  }

#if defined(Z_IOPOLL_HAS_EPOLL)
    __global_iopoll->vtable = &z_iopoll_epoll;
#elif defined(Z_IOPOLL_HAS_KQUEUE)
    __global_iopoll->vtable = &z_iopoll_kqueue;
#else
    #error "IOPoll engine missing"
#endif

  /* Initialize iopoll-engine context */
  ioengine = __global_iopoll->engines;
  while (ncores--) {
    Z_LOG_DEBUG("Initialize iopoll engine %d", __iopoll_engine_id(ioengine));
    if (__iopoll_engine_open(ioengine++)) {
      while (++ncores < __global_iopoll->thread_pool.size)
        __iopoll_engine_close(--ioengine);

      z_thread_pool_close(&(__global_iopoll->thread_pool));
      z_allocator_free(z_system_allocator(), __global_iopoll);
      __global_iopoll = NULL;
      return(5);
    }
  }
  return(0);
}

void z_iopoll_close (void) {
  int i, size = __global_iopoll->thread_pool.size;

  /* clear queue */
  z_thread_pool_stop(&(__global_iopoll->thread_pool));
  for (i = 0; i < size; ++i) {
    __iopoll_engine_close(&(__global_iopoll->engines[i]));
  }
  z_thread_pool_close(&(__global_iopoll->thread_pool));

  /* Dump engine-ctx */
  for (i = 0; i < size; ++i) {
    __iopoll_engine_dump(&(__global_iopoll->engines[i]));
  }

  z_allocator_free(z_system_allocator(), __global_iopoll);
  __global_iopoll = NULL;
}

void z_iopoll_stop (void) {
  z_thread_pool_stop(&(__global_iopoll->thread_pool));
}

void z_iopoll_wait (void) {
  z_thread_pool_wait(&(__global_iopoll->thread_pool));
}

int z_iopoll_add (z_iopoll_entity_t *entity) {
  if (!(entity->iflags8 & Z_IOPOLL_WATCHED)) {
    entity->ioengine = z_thread_pool_balance(&(__global_iopoll->thread_pool));
  }
  return(__iopoll_engine_add(&(__global_iopoll->engines[entity->ioengine]), entity));
}

int z_iopoll_remove (z_iopoll_entity_t *entity) {
  z_iopoll_engine_t *engine = &(__global_iopoll->engines[entity->ioengine]);
  return(__iopoll_engine_remove(engine, entity));
}

int z_iopoll_timer (z_iopoll_entity_t *entity, uint32_t msec) {
  if (!(entity->iflags8 & Z_IOPOLL_WATCHED)) {
    entity->ioengine = z_thread_pool_balance(&(__global_iopoll->thread_pool));
  }
  return(__iopoll_engine_timer(&(__global_iopoll->engines[entity->ioengine]), entity, msec));
}

int z_iopoll_uevent (z_iopoll_entity_t *entity, int enable) {
  if (!(entity->iflags8 & Z_IOPOLL_WATCHED)) {
    entity->ioengine = z_thread_pool_balance(&(__global_iopoll->thread_pool));
  }
  return(__iopoll_engine_uevent(&(__global_iopoll->engines[entity->ioengine]), entity, enable));
}

int z_iopoll_notify (z_iopoll_entity_t *entity) {
  z_iopoll_engine_t *engine = &(__global_iopoll->engines[entity->ioengine]);
  return(__iopoll_engine_notify(engine, entity));
}

int z_iopoll_poll (int detached, const int *is_running) {
  Z_ASSERT(is_running != NULL, "is_running must be a non NULL pointer");
  __global_iopoll->is_running = is_running;

  if (detached)
    return(__iopoll_threads_start(0));

  /* engine[1:] are running on other threads */
  if (__global_iopoll->thread_pool.size > 1) {
    __iopoll_threads_start(1);
  }

  /* engine[0] is running on the main thread */
  __global_iopoll->vtable->poll(&(__global_iopoll->engines[0]));
  z_thread_pool_worker_close(&(__global_iopoll->thread_pool));

  if (__global_iopoll->thread_pool.size > 1)
    __iopoll_threads_wait(1);
  return(0);
}

unsigned int z_iopoll_cores (void) {
  return(__global_iopoll->thread_pool.size);
}

const z_iopoll_stats_t *z_iopoll_stats (int core) {
  return(&(__global_iopoll->engines[core].stats));
}

/* ===========================================================================
 *  PUBLIC I/O Poll entity methods
 */
void z_iopoll_entity_open (z_iopoll_entity_t *entity,
                           const z_iopoll_entity_vtable_t *vtable,
                           int fd)
{
  z_ticket_init(&(entity->lock));
  entity->type = __iopoll_entity_register(vtable);
  entity->iflags8  = Z_IOPOLL_READABLE;
  entity->ioengine = 0;
  entity->uflags8  = 0;
  entity->last_write_sec = 0;
  entity->fd = fd;
}

void z_iopoll_entity_close (z_iopoll_entity_t *entity, int defer) {
  z_ticket_acquire(&(entity->lock));
  entity->iflags8 ^= Z_IOPOLL_HANGUP;
  z_ticket_release(&(entity->lock));
  if (!defer) {
    close(entity->fd);
  }
}

void z_iopoll_set_writable (z_iopoll_entity_t *entity, int writable) {
  uint32_t now = z_time_secs();
  z_ticket_acquire(&(entity->lock));
  if (!!writable != !!(entity->iflags8 & Z_IOPOLL_HAS_DATA)) {
    entity->iflags8 ^= Z_IOPOLL_HAS_DATA;
    if (writable && !(entity->iflags8 & Z_IOPOLL_WRITABLE) &&
        !(entity->iflags8 & Z_IOPOLL_HANGUP))
    {
      z_iopoll_engine_t *engine = &(__global_iopoll->engines[entity->ioengine]);
      entity->iflags8 ^= Z_IOPOLL_WRITABLE;
      __iopoll_engine_add(engine, entity);
    }
  }
  entity->last_write_sec = now;
  z_ticket_release(&(entity->lock));
}

void z_iopoll_add_latencies (z_iopoll_entity_t *entity,
                             uint64_t req_latency,
                             uint64_t resp_latency)
{
  z_iopoll_stats_t *stats = &(__global_iopoll->engines[entity->ioengine].stats);
  z_histogram_add(&(stats->req_latency), req_latency);
  z_histogram_add(&(stats->resp_latency), resp_latency);
}

/* ===========================================================================
 *  PUBLIC I/O Poll entity methods
 */
void z_iopoll_timer_open (z_iopoll_entity_t *entity,
                          const z_iopoll_entity_vtable_t *vtable,
                          int oid)
{
  z_ticket_init(&(entity->lock));
  entity->type = __iopoll_entity_register(vtable);
  entity->iflags8  = Z_IOPOLL_TIMEOUT;
  entity->ioengine = 0;
  entity->uflags8  = 0;
  entity->last_write_sec = 0;
#if defined(Z_IOPOLL_HAS_EPOLL)
  /* epoll uses its own timerfd */
  entity->fd = -1;
#else
  entity->fd = oid;
#endif
}

void z_iopoll_uevent_open (z_iopoll_entity_t *entity,
                           const z_iopoll_entity_vtable_t *vtable,
                           int oid)
{
  z_ticket_init(&(entity->lock));
  entity->type = __iopoll_entity_register(vtable);
  entity->iflags8  = Z_IOPOLL_UEVENT;
  entity->ioengine = 0;
  entity->uflags8  = 0;
  entity->last_write_sec = 0;
#if defined(Z_IOPOLL_HAS_EPOLL)
  /* epoll uses its own timerfd */
  entity->fd = -1;
#else
  entity->fd = oid;
#endif
}
