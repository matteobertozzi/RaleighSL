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
    if (Z_UNLIKELY(vtable->uevent(engine, entity) < 0)) {
      goto _handle_failure;
    }
    now = z_time_micros();
    z_histogram_add(&(engine->stats.event), now - stime);
  }

  /* timeout event */
  if (Z_UNLIKELY(events & Z_IOPOLL_TIMEOUT)) {
    stime = now;
    if (Z_UNLIKELY(vtable->timeout(engine, entity) < 0)) {
      goto _handle_failure;
    }
    now = z_time_micros();
    z_histogram_add(&(engine->stats.timeout), now - stime);
  }

  /* read event */
  if (events & Z_IOPOLL_READABLE) {
    stime = now;
    if (Z_UNLIKELY(vtable->read(engine, entity) < 0)) {
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
      if (Z_UNLIKELY(vtable->write(engine, entity) < 0)) {
        goto _handle_failure;
      }
      now = z_time_micros();
      z_histogram_add(&(engine->stats.iowrite), now - stime);

      if (Z_UNLIKELY(entity->iflags8 & Z_IOPOLL_HAS_DATA &&
          !(entity->iflags8 & Z_IOPOLL_WRITABLE)))
      {
        entity->iflags8 ^= Z_IOPOLL_WRITABLE;
        z_iopoll_engine_add(engine, entity);
      }
    } else if (Z_UNLIKELY(events & Z_IOPOLL_WRITABLE && __write_time_elapsed(entity, now))) {
      Z_LOG_DEBUG("switch state %u %u diff %u (too many write events)",
                  (uint32_t)(now & 0xffffffffull), entity->last_write_ts,
                  (uint32_t)(now & 0xffffffffull) - entity->last_write_ts);

      /* Apply the changes after 1sec of no data, to avoid too may context switch */
      if (!(entity->iflags8 & Z_IOPOLL_HAS_DATA)) {
        entity->iflags8 ^= Z_IOPOLL_WRITABLE;
        z_iopoll_engine_add(engine, entity);
      }
    }
  }

  if (Z_UNLIKELY(entity->iflags8 & Z_IOPOLL_HANGUP)) {
    int fd = entity->fd;
    z_iopoll_engine_remove(engine, entity);
    vtable->close(engine, entity);
    close(fd);
    return;
  }

  return;

_handle_failure:
  z_iopoll_engine_remove(engine, entity);
  vtable->close(engine, entity);
  return;
}

/* ===========================================================================
 *  PRIVATE I/O Poll Stats
 */
#define __STATS_HISTO_NBOUNDS       z_fix_array_size(__STATS_HISTO_BOUNDS)
static const uint64_t __STATS_HISTO_BOUNDS[20] = {
  Z_TIME_USEC(5),   Z_TIME_USEC(10),  Z_TIME_USEC(25),  Z_TIME_USEC(60),
  Z_TIME_USEC(80),  Z_TIME_USEC(125), Z_TIME_USEC(200), Z_TIME_USEC(350),
  Z_TIME_MSEC(500), Z_TIME_USEC(750), Z_TIME_MSEC(5),   Z_TIME_MSEC(10),
  Z_TIME_MSEC(25),  Z_TIME_MSEC(75),  Z_TIME_MSEC(100), Z_TIME_MSEC(250),
  Z_TIME_MSEC(500), Z_TIME_MSEC(750), Z_TIME_SEC(1),    0xffffffffffffffffll,
};

/* ===========================================================================
 *  PUBLIC I/O Poll engine methods
 */
int z_iopoll_engine_open (z_iopoll_engine_t *engine) {
  z_iopoll_stats_t *stats = &(engine->stats);

  /* Initialize I/O Poll Load */
  memset(&(stats->ioload), 0, sizeof(z_iopoll_load_t));

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

  if (z_iopoll_engine.open(engine)) {
    return(1);
  }
  return(0);
}

void z_iopoll_engine_close (z_iopoll_engine_t *engine) {
  z_iopoll_engine.close(engine);
}

void z_iopoll_engine_dump (z_iopoll_engine_t *engine, FILE *stream) {
  z_iopoll_stats_t *stats = &(engine->stats);

  printf("IOPoll Stats\n");
  printf("==================================\n");
  fprintf(stream, "IOWait");
  z_histogram_dump(&(stats->iowait), stream, z_human_dtime);
  fprintf(stream, "\n");

  fprintf(stream, "IORead ");
  z_histogram_dump(&(stats->ioread), stream, z_human_dtime);
  fprintf(stream, "\n");

  fprintf(stream, "IOWrite ");
  z_histogram_dump(&(stats->iowrite), stream, z_human_dtime);
  fprintf(stream, "\n");

  fprintf(stream, "Event ");
  z_histogram_dump(&(stats->event), stream, z_human_dtime);
  fprintf(stream, "\n");

  fprintf(stream, "Timeout ");
  z_histogram_dump(&(stats->timeout), stream, z_human_dtime);
  fprintf(stream, "\n");
}

void z_iopoll_stats_add_events (z_iopoll_engine_t *engine,
                                int nevents,
                                uint64_t idle_usec)
{
  z_iopoll_load_t *ioload = &((engine)->stats.ioload);
  z_histogram_t *iowait = &((engine)->stats.iowait);
  int tail;

  tail = ioload->tail;
  ioload->max_events = z_max(nevents, ioload->max_events);
  if ((ioload->events[tail] + nevents) <= 0xff) {
    ioload->events[tail] += nevents & 0xffff;
    ioload->idle[tail]   += idle_usec & 0xffffffff;
  } else {
    tail = ioload->tail  = (tail + 1) % 6;
    ioload->events[tail] = nevents & 0xffff;
    ioload->idle[tail]   = idle_usec & 0xffffffff;
    ioload->active[tail] = 0;
  }

  z_histogram_add(iowait, idle_usec);
}

float z_iopoll_engine_load (z_iopoll_engine_t *self) {
  const z_iopoll_load_t *load = &(self->stats.ioload);
  const uint32_t *actv = load->active;
  const uint32_t *idle = load->idle;
  float total_active;
  float total_idle;

  total_active = 1 + actv[0] + actv[1] + actv[2] + actv[3] + actv[4] + actv[5];
  total_idle   = 1 + idle[0] + idle[1] + idle[2] + idle[3] + idle[4] + idle[5];

  return((total_active * 100.0f) / (total_active + total_idle));
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

void z_iopoll_set_writable (z_iopoll_engine_t *engine,
                            z_iopoll_entity_t *entity,
                            int writable)
{
  entity->last_write_ts = z_time_micros() & 0xffffffff;
  if (!!writable != !!(entity->iflags8 & Z_IOPOLL_HAS_DATA)) {
    entity->iflags8 ^= Z_IOPOLL_HAS_DATA;
  }
  if (writable && !(entity->iflags8 & Z_IOPOLL_WRITABLE) &&
      !(entity->iflags8 & Z_IOPOLL_HANGUP))
  {
    entity->iflags8 ^= Z_IOPOLL_WRITABLE;
    z_iopoll_engine_add(engine, entity);
  }
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
