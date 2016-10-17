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
#include <zcl/humans.h>
#include <zcl/debug.h>
#include <zcl/time.h>

#include <string.h>
#include <unistd.h>

#define __time_u32_usec(nsec)     ((uint32_t)(Z_TIME_NANOS_TO_MICROS(nsec) / 0xffffffff))

/* ============================================================================
 *  PRIVATE I/O Poll registration
 */
static const z_iopoll_entity_vtable_t *__iopoll_entity_vtables[32] = {
  /*  0- 7 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  /*  8-15 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  /* 16-23 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  /* 24-31 */ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};

static uint8_t __iopoll_entity_register (const z_iopoll_entity_vtable_t *vtable) {
  uint8_t vtables_count = z_fix_array_size(__iopoll_entity_vtables);
  uint8_t i;
  for (i = 0; i < vtables_count; ++i) {
    if (__iopoll_entity_vtables[i] == vtable || __iopoll_entity_vtables[i] == NULL) {
      __iopoll_entity_vtables[i] = vtable;
      return(i);
    }
  }
  Z_LOG_FATAL("Too many iopoll-entities registered: %d", i);
  return(0xff);
}

static void __iopoll_entity_open (z_iopoll_entity_t *entity,
                                  const z_iopoll_entity_vtable_t *vtable,
                                  uint8_t entity_type,
                                  int fd)
{
  entity->type = __iopoll_entity_register(vtable);
  entity->iflags8  = entity_type;
  entity->ioengine = 0;
  entity->uflags8  = 0;
  entity->last_write_ts = 0;
  entity->fd = fd;
  entity->uflags32 = 0;
}

/* ============================================================================
 *  INTERNAL methods called by the engines
 */
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
  stime = now = z_time_monotonic();
  if (Z_UNLIKELY(events & Z_IOPOLL_UEVENT)) {
    if (Z_UNLIKELY(vtable->uevent(engine, entity) < 0)) {
      goto _handle_failure;
    }
    now = z_time_monotonic();
    z_histogram_add(&(engine->stats.event), now - stime);
  }

  /* timeout event */
  if (Z_UNLIKELY(events & Z_IOPOLL_TIMEOUT)) {
    stime = now;
    if (Z_UNLIKELY(vtable->timeout(engine, entity) < 0)) {
      goto _handle_failure;
    }
    now = z_time_monotonic();
    z_histogram_add(&(engine->stats.timeout), now - stime);
  }

  /* read event */
  if (events & Z_IOPOLL_READABLE) {
    stime = now;
    if (Z_UNLIKELY(vtable->read(engine, entity) < 0)) {
      goto _handle_failure;
    }
    now = z_time_monotonic();
    z_histogram_add(&(engine->stats.ioread), now - stime);
  }

  /* write event */
  const int has_data = entity->iflags8 & Z_IOPOLL_HAS_DATA;
  if (has_data || events & Z_IOPOLL_WRITABLE) {
    if (Z_LIKELY(has_data)) {
      stime = now;
      if (Z_UNLIKELY(vtable->write(engine, entity) < 0)) {
        goto _handle_failure;
      }
      now = z_time_monotonic();
      z_histogram_add(&(engine->stats.iowrite), now - stime);

      if (Z_UNLIKELY(entity->iflags8 & Z_IOPOLL_HAS_DATA &&
          !(entity->iflags8 & Z_IOPOLL_WRITABLE)))
      {
        entity->iflags8 ^= Z_IOPOLL_WRITABLE;
        z_iopoll_engine_add(engine, entity);
      }
    } else if (Z_UNLIKELY((events & Z_IOPOLL_WRITABLE) &&
              (Z_TIME_NANOS_TO_SECONDS(now) - entity->last_write_ts) > 1)) {
      Z_LOG_DEBUG("switch state %u %u diff %usec (too many write events)",
                  Z_TIME_NANOS_TO_SECONDS(now), entity->last_write_ts,
                  Z_TIME_NANOS_TO_SECONDS(now) - entity->last_write_ts);

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
static const uint64_t __STATS_HISTO_BOUNDS[21] = {
  Z_TIME_MICROS_TO_NANOS(5),   Z_TIME_MICROS_TO_NANOS(10),  Z_TIME_MICROS_TO_NANOS(25),
  Z_TIME_MICROS_TO_NANOS(60),  Z_TIME_MICROS_TO_NANOS(80),  Z_TIME_MICROS_TO_NANOS(125),
  Z_TIME_MICROS_TO_NANOS(200), Z_TIME_MICROS_TO_NANOS(350), Z_TIME_MICROS_TO_NANOS(500),
  Z_TIME_MICROS_TO_NANOS(750), Z_TIME_MILLIS_TO_NANOS(5),   Z_TIME_MILLIS_TO_NANOS(10),
  Z_TIME_MILLIS_TO_NANOS(25),  Z_TIME_MILLIS_TO_NANOS(75),  Z_TIME_MILLIS_TO_NANOS(100),
  Z_TIME_MILLIS_TO_NANOS(250), Z_TIME_MILLIS_TO_NANOS(350), Z_TIME_MILLIS_TO_NANOS(500),
  Z_TIME_MILLIS_TO_NANOS(750), Z_TIME_SECONDS_TO_NANOS(1),  0xffffffffffffffffull,
};

/* ===========================================================================
 *  PUBLIC I/O Poll engine methods
 */
int z_iopoll_engine_open (z_iopoll_engine_t *engine) {
  z_iopoll_stats_t *stats = &(engine->stats);

  /* Initialize I/O Poll Load */
  memset(&(engine->load), 0, sizeof(z_iopoll_load_t));

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

  return z_iopoll_engine.open(engine);
}

void z_iopoll_engine_close (z_iopoll_engine_t *engine) {
  z_iopoll_engine.close(engine);
}

static void __iopoll_load_dump (z_iopoll_load_t *self, FILE *stream) {
  char buf0[16], buf1[16];
  int i;
  for (i = 0; i < 3; ++i) {
    const int index = (self->tail + i) % 3;
    fprintf(stream, " - [%d] events %u/%u idle %s active %s\n",
            i, self->events[index], self->max_events,
            z_human_time(buf0, sizeof(buf0), self->idle[index]),
            z_human_time(buf1, sizeof(buf1), self->active[index]));
  }
}

void z_iopoll_engine_dump (z_iopoll_engine_t *engine, FILE *stream) {
  z_iopoll_stats_t *stats = &(engine->stats);

  printf("IOPoll Stats\n");
  printf("==================================\n");
  fprintf(stream, "IOLoad %.2f%%\n", z_iopoll_engine_load(engine));
  __iopoll_load_dump(&(engine->load), stream);
  fprintf(stream, "\n");

  fprintf(stream, "IOWait");
  z_histogram_dump(&(stats->iowait), stream, z_human_time_d);
  fprintf(stream, "\n");

  fprintf(stream, "IORead ");
  z_histogram_dump(&(stats->ioread), stream, z_human_time_d);
  fprintf(stream, "\n");

  fprintf(stream, "IOWrite ");
  z_histogram_dump(&(stats->iowrite), stream, z_human_time_d);
  fprintf(stream, "\n");

  fprintf(stream, "Event ");
  z_histogram_dump(&(stats->event), stream, z_human_time_d);
  fprintf(stream, "\n");

  fprintf(stream, "Timeout ");
  z_histogram_dump(&(stats->timeout), stream, z_human_time_d);
  fprintf(stream, "\n");
}

void z_iopoll_stats_add_events (z_iopoll_engine_t *engine,
                                int nevents,
                                uint64_t idle_nsec)
{
  z_iopoll_load_t *ioload = &((engine)->load);
  z_histogram_t *iowait = &((engine)->stats.iowait);
  int tail;

  tail = ioload->tail;
  ioload->max_events = z_max(nevents & 0xff, ioload->max_events);
  if ((ioload->events[tail] + nevents) < 0xffff) {
    ioload->events[tail] += nevents;
    ioload->idle[tail]   += idle_nsec;
  } else {
    tail = ioload->tail  = (tail + 1) % 3;
    ioload->events[tail] = nevents & 0xff;
    ioload->idle[tail]   = idle_nsec;
    ioload->active[tail] = 0;
  }

  z_histogram_add(iowait, idle_nsec);
}

float z_iopoll_engine_load (z_iopoll_engine_t *self) {
  const z_iopoll_load_t *load = &(self->load);
  const uint64_t *actv = load->active;
  const uint64_t *idle = load->idle;
  float total_active;
  float total_idle;

  total_active = 1 + actv[0] + actv[1] + actv[2];
  total_idle   = 1 + idle[0] + idle[1] + idle[2];

  return((total_active * 100.0f) / (total_active + total_idle));
}

/* ===========================================================================
 *  PUBLIC I/O Poll entity methods
 */
void z_iopoll_entity_open (z_iopoll_entity_t *entity,
                           const z_iopoll_entity_vtable_t *vtable,
                           int fd)
{
  __iopoll_entity_open(entity, vtable, Z_IOPOLL_READABLE, fd);
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
  entity->last_write_ts = Z_TIME_NANOS_TO_SECONDS(z_time_monotonic());
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
 *  PUBLIC Timer/UEvents entity methods
 */
void z_iopoll_timer_open (z_iopoll_entity_t *entity,
                          const z_iopoll_entity_vtable_t *vtable,
                          int oid)
{
#if defined(Z_IOPOLL_HAS_EPOLL)
  /* epoll uses its own timerfd */
  oid = -1;
#endif
  __iopoll_entity_open(entity, vtable, Z_IOPOLL_TIMEOUT, oid);
}

void z_iopoll_uevent_open (z_iopoll_entity_t *entity,
                           const z_iopoll_entity_vtable_t *vtable,
                           int oid)
{
#if defined(Z_IOPOLL_HAS_EPOLL)
  /* epoll uses its own timerfd */
  oid = -1;
#endif
  __iopoll_entity_open(entity, vtable, Z_IOPOLL_UEVENT, oid);
}

/* ============================================================================
 *  I/O Poll Entity I/O
 */
static ssize_t __iopoll_entity_read (void *udata, void *buffer, size_t bufsize) {
  return z_fd_read(Z_IOPOLL_ENTITY(udata)->fd, buffer, bufsize);
}

static ssize_t __iopoll_entity_write (void *udata, const void *buffer, size_t bufsize) {
  return z_fd_write(Z_IOPOLL_ENTITY(udata)->fd, buffer, bufsize);
}

static ssize_t __iopoll_entity_readv (void *udata, const struct iovec *iov, int iovcnt) {
  return z_fd_readv(Z_IOPOLL_ENTITY(udata)->fd, iov, iovcnt);
}

static ssize_t __iopoll_entity_writev (void *udata, const struct iovec *iov, int iovcnt) {
  return z_fd_writev(Z_IOPOLL_ENTITY(udata)->fd, iov, iovcnt);
}

const z_io_seq_vtable_t z_iopoll_entity_raw_io_seq_vtable = {
  .read   = __iopoll_entity_read,
  .write  = __iopoll_entity_write,
  .readv  = __iopoll_entity_readv,
  .writev = __iopoll_entity_writev,
};
