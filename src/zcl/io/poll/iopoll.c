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

#include <zcl/allocator.h>
#include <zcl/global.h>
#include <zcl/iopoll.h>
#include <zcl/debug.h>
#include <zcl/time.h>
#include <zcl/fd.h>

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
 *  PRIVATE iopoll-engine macros
 */
#define __iopoll_engine_add(engine, entity)                         \
  z_iopoll_engine.add(engine, entity)

#define __iopoll_engine_remove(engine, entity)                      \
  z_iopoll_engine.remove(engine, entity)

#define __iopoll_engine_timer(engine, entity, msec)                 \
  z_iopoll_engine.timer(engine, entity, msec)

#define __iopoll_engine_uevent(engine, entity, enable)              \
  z_iopoll_engine.uevent(engine, entity, enable)

#define __iopoll_engine_notify(engine, entity)                      \
  z_iopoll_engine.notify(engine, entity)

#define __iopoll_engine_poll(engine)                                \
  z_iopoll_engine.poll(engine)

/* ============================================================================
 *  INTERNAL methods called by the engines
 */
#define __write_time_elapsed(entity, now)                                 \
 ((((now) & 0xffffffffull) - (entity)->last_write_ts) > 1000000ull)

void z_iopoll_process (z_iopoll_engine_t *engine,
                       z_iopoll_entity_t *entity,
                       uint32_t events)
{
  const z_iopoll_entity_vtable_t *vtable = __iopoll_entity_vtables[entity->type];
  uint64_t stime, now;

  /* hangup event */
  if (Z_UNLIKELY(events & Z_IOPOLL_HANGUP)) {
    goto _handle_failure;
  }

  /* user event */
  stime = now = z_time_micros();
  if (Z_UNLIKELY(events & Z_IOPOLL_UEVENT)) {
    if (Z_UNLIKELY(vtable->uevent(entity) < 0)) {
      goto _handle_failure;
    }
    now = z_time_micros();
    z_histogram_add(&(engine->stats.event), now - stime);
  }

  /* timeout event */
  if (Z_UNLIKELY(events & Z_IOPOLL_TIMEOUT)) {
    stime = now;
    if (Z_UNLIKELY(vtable->timeout(entity) < 0)) {
      goto _handle_failure;
    }
    now = z_time_micros();
    z_histogram_add(&(engine->stats.timeout), now - stime);
  }

  /* read event */
  if (events & Z_IOPOLL_READABLE) {
    stime = now;
    if (Z_UNLIKELY(vtable->read(entity) < 0)) {
      goto _handle_failure;
    }
    now = z_time_micros();
    z_histogram_add(&(engine->stats.ioread), now - stime);
  }

  /* write event */
  int has_data = entity->iflags8 & Z_IOPOLL_HAS_DATA;
  if (has_data || events & Z_IOPOLL_WRITABLE) {
    if (Z_LIKELY(has_data)) {
      stime = now;
      if (Z_UNLIKELY(vtable->write(entity) < 0)) {
        goto _handle_failure;
      }
      now = z_time_micros();
      z_histogram_add(&(engine->stats.iowrite), now - stime);
    } else if (Z_UNLIKELY(events & Z_IOPOLL_WRITABLE && __write_time_elapsed(entity, now))) {
      Z_LOG_DEBUG("switch state %u %u diff %u (too many write events)\n",
                  (uint32_t)(now & 0xffffffffull), entity->last_write_ts,
                  (uint32_t)(now & 0xffffffffull) - entity->last_write_ts);

      /* Apply the changes after 1sec of no data, to avoid too may context switch */
      if (!(entity->iflags8 & Z_IOPOLL_HAS_DATA)) {
        entity->iflags8 ^= Z_IOPOLL_WRITABLE;
        __iopoll_engine_add(engine, entity);
      }
    }
  }

  if (Z_UNLIKELY(entity->iflags8 & Z_IOPOLL_HANGUP)) {
    int fd = entity->fd;
    __iopoll_engine_remove(engine, entity);
    vtable->close(entity);
    close(fd);
    return;
  }

  return;

_handle_failure:
  __iopoll_engine_remove(engine, entity);
  vtable->close(entity);
  return;
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
 *  PUBLIC I/O Poll engine methods
 */
int z_iopoll_engine_open (z_iopoll_engine_t *engine) {
  z_iopoll_stats_t *stats = &(engine->stats);

  /* Initialize I/O Poll Histogram */
  z_histogram_init(&(stats->iowait),  __STATS_HISTO_BOUNDS,
                   stats->iowait_events,  __STATS_HISTO_NBOUNDS);
  z_histogram_init(&(stats->ioread),  __STATS_HISTO_BOUNDS,
                   stats->ioread_events,  __STATS_HISTO_NBOUNDS);
  z_histogram_init(&(stats->iowrite), __STATS_HISTO_BOUNDS,
                   stats->iowrite_events, __STATS_HISTO_NBOUNDS);
  z_histogram_init(&(stats->event), __STATS_HISTO_BOUNDS,
                   stats->event_events, __STATS_HISTO_NBOUNDS);
  z_histogram_init(&(stats->timeout), __STATS_HISTO_BOUNDS,
                   stats->timeout_events, __STATS_HISTO_NBOUNDS);
  z_histogram_init(&(stats->req_latency), __STATS_HISTO_BOUNDS,
                   stats->req_latency_events, __STATS_HISTO_NBOUNDS);
  z_histogram_init(&(stats->resp_latency), __STATS_HISTO_BOUNDS,
                   stats->resp_latency_events, __STATS_HISTO_NBOUNDS);

  if (z_iopoll_engine.open(engine)) {
    return(1);
  }
  return(0);
}

void z_iopoll_engine_close (z_iopoll_engine_t *engine) {
  z_iopoll_engine.close(engine);
}

void z_iopoll_engine_dump (z_iopoll_engine_t *engine) {
  z_iopoll_stats_t *stats = &(engine->stats);

  printf("IOPoll Stats\n");
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

  fprintf(stdout, "Event ");
  z_histogram_dump(&(stats->event), stdout, z_human_dtime);
  fprintf(stdout, "\n");

  fprintf(stdout, "Timeout ");
  z_histogram_dump(&(stats->timeout), stdout, z_human_dtime);
  fprintf(stdout, "\n");

  fprintf(stdout, "Req Latency ");
  z_histogram_dump(&(stats->req_latency), stdout, z_human_dtime);
  fprintf(stdout, "\n");

  fprintf(stdout, "Resp Latency ");
  z_histogram_dump(&(stats->resp_latency), stdout, z_human_dtime);
  fprintf(stdout, "\n");
}

/* ===========================================================================
 *  PUBLIC Global I/O Poll methods
 */
int z_iopoll_add (z_iopoll_entity_t *entity) {
  if (!(entity->iflags8 & Z_IOPOLL_WATCHED)) {
    entity->ioengine = z_global_balance();
    Z_LOG_DEBUG("add io-entity %p to engine %u", entity, entity->ioengine);
  }
  return(__iopoll_engine_add(z_global_iopoll_at(entity->ioengine), entity));
}

int z_iopoll_remove (z_iopoll_entity_t *entity) {
  z_iopoll_engine_t *engine = z_global_iopoll_at(entity->ioengine);
  return(__iopoll_engine_remove(engine, entity));
}

int z_iopoll_timer (z_iopoll_entity_t *entity, uint32_t msec) {
  if (!(entity->iflags8 & Z_IOPOLL_WATCHED)) {
    entity->ioengine = z_global_balance();
    Z_LOG_DEBUG("add timer %p to engine %u", entity, entity->ioengine);
  }
  return(__iopoll_engine_timer(z_global_iopoll_at(entity->ioengine), entity, msec));
}

int z_iopoll_uevent (z_iopoll_entity_t *entity, int enable) {
  if (!(entity->iflags8 & Z_IOPOLL_WATCHED)) {
    entity->ioengine = z_global_balance();
    Z_LOG_DEBUG("add uevent %p to engine %u", entity, entity->ioengine);
  }
  return(__iopoll_engine_uevent(z_global_iopoll_at(entity->ioengine), entity, enable));
}

int z_iopoll_notify (z_iopoll_entity_t *entity) {
  z_iopoll_engine_t *engine = z_global_iopoll_at(entity->ioengine);
  return(__iopoll_engine_notify(engine, entity));
}

/* ===========================================================================
 *  PUBLIC I/O Poll entity methods
 */
void z_iopoll_entity_open (z_iopoll_entity_t *entity,
                           const z_iopoll_entity_vtable_t *vtable,
                           int fd)
{
  entity->type = __iopoll_entity_register(vtable);
  entity->iflags8  = Z_IOPOLL_READABLE;
  entity->ioengine = 0;
  entity->uflags8  = 0;
  entity->last_write_ts = 0;
  entity->fd = fd;
}

void z_iopoll_entity_close (z_iopoll_entity_t *entity, int defer) {
  entity->iflags8 ^= Z_IOPOLL_HANGUP;
  if (!defer) {
    close(entity->fd);
  }
}

void z_iopoll_set_data_available (z_iopoll_entity_t *entity, int has_data) {
  if (has_data) {
    entity->iflags8 |= Z_IOPOLL_HAS_DATA;
  } else {
    entity->iflags8 &= ~Z_IOPOLL_HAS_DATA;
  }
}

void z_iopoll_set_writable (z_iopoll_entity_t *entity, int writable) {
  entity->last_write_ts = z_time_micros() & 0xffffffff;
  if (!!writable != !!(entity->iflags8 & Z_IOPOLL_HAS_DATA)) {
    entity->iflags8 ^= Z_IOPOLL_HAS_DATA;
  }
  if (writable && !(entity->iflags8 & Z_IOPOLL_WRITABLE) &&
      !(entity->iflags8 & Z_IOPOLL_HANGUP))
  {
    z_iopoll_engine_t *engine = z_global_iopoll_at(entity->ioengine);
    entity->iflags8 ^= Z_IOPOLL_WRITABLE;
    __iopoll_engine_add(engine, entity);
  }
}

void z_iopoll_add_latencies (z_iopoll_entity_t *entity,
                             uint64_t req_latency,
                             uint64_t resp_latency)
{
  z_iopoll_stats_t *stats = &(z_global_iopoll_at(entity->ioengine)->stats);
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
  entity->type = __iopoll_entity_register(vtable);
  entity->iflags8  = Z_IOPOLL_TIMEOUT;
  entity->ioengine = 0;
  entity->uflags8  = 0;
  entity->last_write_ts = 0;
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
  entity->type = __iopoll_entity_register(vtable);
  entity->iflags8  = Z_IOPOLL_UEVENT;
  entity->ioengine = 0;
  entity->uflags8  = 0;
  entity->last_write_ts = 0;
#if defined(Z_IOPOLL_HAS_EPOLL)
  /* epoll uses its own timerfd */
  entity->fd = -1;
#else
  entity->fd = oid;
#endif
}
