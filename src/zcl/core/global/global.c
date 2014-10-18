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

#include <zcl/global.h>
#include <zcl/memory.h>
#include <zcl/thread.h>
#include <zcl/iopoll.h>
#include <zcl/debug.h>
#include <zcl/time.h>
#include <zcl/math.h>

Z_TYPEDEF_STRUCT(z_global_context)
Z_TYPEDEF_STRUCT(z_event_loop)

struct z_event_loop {
  /* iopoll-engine */
  z_iopoll_engine_t iopoll;
  z_iopoll_entity_t entity_event;
  z_iopoll_entity_t entity_timeout;
/* (26 * 64) = 1664 */

  /* memory */
  z_memory_t    memory;
  z_allocator_t allocator;

  /* internals */
  int           core;
  int           is_running;
  int           is_detached;
  z_thread_t    thread;
  uint8_t       __pad[Z_CACHELINE_PAD_FIELDS(int, int, int, z_thread_t)];
} __attribute__((packed));

struct z_global_context {
  uint32_t eloop_data;
  uint32_t eloop_size;
  uint32_t user_data;
  uint32_t ctx_size;
  uint64_t  __pad0[6];

  int *    is_running;
  uint64_t  __pad1[7];

  uint64_t balance;
  uint64_t __pad2[7];

  z_global_conf_t conf;
  uint8_t  __pad3[Z_CACHELINE_PAD(sizeof(z_global_conf_t))];

  uint8_t  data[1];
} __attribute__((packed));

/*
 * +--------------------+
 * | z_global_context_t |
 * +--------------------+ ctx->data + (core * sizeof(z_event_loop_t))
 * |  z_event_loop_t 0  |
 * |  z_event_loop_t 1  |
 * |  z_event_loop_t N  |
 * +--------------------+ ctx->data + ctx->eloop_data + (core * ctx->eloop_size)
 * |  events ring 0     |
 * |  mmpool 0          |
 * +--------------------+
 * |  events ring 1     |
 * |  mmpool 1          |
 * +--------------------+
 * |  events ring N     |
 * |  mmpool N          |
 * +--------------------+ ctx->data + ctx->user_data
 * |  udata             |
 * +--------------------+
 */

/* ============================================================================
 *  PRIVATE global-context instance
 */
static z_global_context_t *__global_ctx = NULL;

#define __ctx_eloop(ctx, core)                                                \
  Z_CAST(z_event_loop_t, (ctx)->data + ((core) * sizeof(z_event_loop_t)))

#define __ctx_eloop_data(ctx, core)                                           \
  ((ctx)->eloop_data + ((core) * (ctx)->eloop_size))

#define __ctx_event_ring_offset(ctx, core)                                    \
  (__ctx_eloop_data(ctx, core) + 0)

#define __ctx_mmpool_offset(ctx, core)                                        \
  (__ctx_eloop_data(ctx, core) + (ctx)->conf.events_ring_size)

#define __global_eloop(core)        __ctx_eloop(__global_ctx, core)

/* ============================================================================
 *  PRIVATE cpu-context thread-local lookups
 */
static __thread z_event_loop_t *__local_event_loop = NULL;
static z_event_loop_t *__set_local_event_loop (void) {
  z_event_loop_t *eloop;
  int not_found = 1;
  uint32_t ncores;
  z_thread_t tid;

  z_thread_self(&tid);
  ncores = __global_ctx->conf.ncores;
  while (ncores--) {
    eloop = __global_eloop(ncores);
    if (z_thread_equal(&(eloop->thread), &tid)) {
      not_found = 0;
      break;
    }
  }

  if (not_found) {
    int core = 0;
    z_thread_get_core(&tid, &core);
    ncores = __global_ctx->conf.ncores;
    eloop = __global_eloop(core % ncores);
  }

  Z_LOG_DEBUG("Lookup Local CPU-ctx for thread %ld (CPU ID %u Local-Thread %d)",
              (long)tid, eloop->core, !not_found);
  __local_event_loop = eloop;
  return(eloop);
}

#define __current_event_loop()                          \
  (Z_LIKELY(__local_event_loop != NULL) ?               \
    __local_event_loop : __set_local_event_loop())

/* ============================================================================
 *  PRIVATE Event Loop (IO-Entity)
 */
static int __event_loop_event (z_iopoll_entity_t *entity) {
  //z_event_loop_t *eloop = z_container_of(entity, z_event_loop_t, entity_event);
  //Z_DEBUG("EVENT ON ELOOP %p", eloop);
#if 0
  z_iopoll_engine.notify(&(eloop->iopoll), &(eloop->entity_event));
#endif
  return(0);
}

static int __event_loop_timeout (z_iopoll_entity_t *entity) {
  z_event_loop_t *eloop = z_container_of(entity, z_event_loop_t, entity_timeout);
  Z_DEBUG("TIMEOUT ON ELOOP %p", eloop);
  return(0);
}

const z_iopoll_entity_vtable_t __event_loop_vtable = {
  .read     = NULL,
  .write    = NULL,
  .uevent   = __event_loop_event,
  .timeout  = __event_loop_timeout,
  .close    = NULL,
};

static void *__event_loop_exec (void *args) {
#if 0
  z_event_loop_t *eloop = Z_CAST(z_event_loop_t, args);
  uint64_t st;

  eloop->events[0] = 0;
  st = z_time_micros();
  z_iopoll_engine.notify(&(eloop->iopoll), &(eloop->entity_event));
#endif
  z_iopoll_engine.poll(&(Z_CAST(z_event_loop_t, args)->iopoll));
#if 0
  char buffer[64];
  float sec = (z_time_micros() - st) / 1000000.0f;
  z_human_dops(buffer, sizeof(buffer), eloop->events[0] / sec);
  Z_DEBUG("%d NEVENTS %.3fsec %s/sec", eloop->core, sec, buffer);
#endif
  return(NULL);
}

/* ============================================================================
 *  PUBLIC Event Loop Allocator
 */
static void *__event_loop_mmalloc (z_allocator_t *self, size_t size) {
  void *ptr;
  ptr = z_sys_alloc(size);
  if (Z_LIKELY(ptr != NULL)) {
    self->extern_alloc++;
    self->extern_used += size;
  }
  return(ptr);
}

static void __event_loop_mmfree (z_allocator_t *self, void *ptr, size_t size) {
  self->extern_used -= size;
  z_sys_free(ptr, size);
}

/* ============================================================================
 *  PUBLIC Event Loop
 */
void z_event_loop_dump (z_event_loop_t *self) {
#if 1
  fprintf(stdout, "==================================\n");
  fprintf(stderr, " ENGINE %d\n", self->core);
  fprintf(stdout, "==================================\n");

  fprintf(stdout, "Memory Stats\n");
  fprintf(stdout, "==================================\n");
  fprintf(stderr, "  pool alloc %"PRIu64"\n", self->allocator.pool_alloc);
  fprintf(stderr, "  pool used  %"PRIu64"\n", self->allocator.pool_used);
  fprintf(stderr, "extern alloc %"PRIu64"\n", self->allocator.extern_alloc);
  fprintf(stderr, "extern used  %"PRIu64"\n", self->allocator.extern_used);
  z_memory_stats_dump(&(self->memory), stdout);
  fprintf(stdout, "\n");

  z_iopoll_engine_dump(&(self->iopoll));
#endif
}

int z_event_loop_open (z_event_loop_t *self,
                       z_global_context_t *ctx,
                       uint32_t core)
{
  /* Initialize iopoll-engine */
  z_iopoll_engine_open(&(self->iopoll));
  z_iopoll_uevent_open(&(self->entity_event), &__event_loop_vtable, core);
  self->entity_timeout.ioengine = self->core;
  z_iopoll_timer_open(&(self->entity_timeout), &__event_loop_vtable, core);
  self->entity_timeout.ioengine = self->core;

  /* Initialize task-queue */

  /* Initialize memory */
  self->allocator.mmalloc = __event_loop_mmalloc;
  self->allocator.mmfree  = __event_loop_mmfree;
  self->allocator.pool_alloc = 0;
  self->allocator.pool_used  = 0;
  self->allocator.extern_alloc = 0;
  self->allocator.extern_used  = 0;
  self->allocator.mmpool = ctx->data + __ctx_mmpool_offset(ctx, core);
  self->allocator.udata  = self;
  z_memory_open(&(self->memory), &(self->allocator));

  /* Initialize internals */
  self->core = core;
  self->is_running = 0;
  self->is_detached = 0;
  return(0);
}

void z_event_loop_close (z_event_loop_t *self) {
  z_iopoll_engine.uevent(&(self->iopoll), &(self->entity_event), 0);
  z_iopoll_engine_close(&(self->iopoll));
  z_memory_close(&(self->memory));

  z_event_loop_dump(self);
}

int z_event_loop_start (z_event_loop_t *self, int start_thread) {
  /* add event-loop entities to io-poll */
  z_iopoll_engine.uevent(&(self->iopoll), &(self->entity_event), 1);

  /* start new thread if required */
  if (start_thread) {
    if (z_thread_start(&(self->thread), __event_loop_exec, self)) {
      self->is_running = 0;
      return(1);
    }

    z_thread_bind_to_core(&(self->thread), self->core);
    self->is_detached = 1;
    self->is_running = 1;
  } else {
    z_thread_self(&(self->thread));
    z_thread_bind_to_core(&(self->thread), self->core);
    self->is_running = 1;
    __event_loop_exec(self);
  }
  return(0);
}

int z_event_loop_stop (z_event_loop_t *self) {
  self->is_running = 0;
  return(z_iopoll_engine.notify(&(self->iopoll), &(self->entity_event)));
}

void z_event_loop_wait (z_event_loop_t *self) {
  z_thread_join(&(self->thread));
}

#define __PAGE_ALIGN        4096

/* ============================================================================
 *  PUBLIC global-context methods
 */
int z_global_context_open (z_global_conf_t *conf) {
  z_global_context_t *gctx;
  uint32_t eloop_size;
  uint32_t gctx_size;
  uint32_t i;

  z_debug_open();

  /* conf: cores */
  conf->ncores = (conf->ncores > 0 ? conf->ncores : z_system_processors());
  conf->ncores = z_max(conf->ncores, 1);
  conf->ncores = z_min(conf->ncores, 0xff);

  /* conf: memory pool */
  conf->mmpool_base_size = z_align_pow2(z_align_up(conf->mmpool_base_size, __PAGE_ALIGN));
  conf->mmpool_page_size = z_align_pow2(z_align_up(conf->mmpool_page_size, __PAGE_ALIGN));
  conf->mmpool_block_min = z_align_pow2(conf->mmpool_block_min);
  conf->mmpoll_block_max = z_align_pow2(conf->mmpoll_block_max);

  /* conf: events */
  conf->events_ring_size = z_align_pow2(z_align_up(conf->events_ring_size, __PAGE_ALIGN));

  /* calculate eloop and gctx size */
  eloop_size  = conf->mmpool_base_size;
  eloop_size += conf->events_ring_size;
  Z_ASSERT((eloop_size & (__PAGE_ALIGN - 1)) == 0, "must be aligned to a page %u", eloop_size);

  gctx_size  = sizeof(z_global_context_t) - 1;
  gctx_size += conf->ncores * sizeof(z_event_loop_t);
  gctx_size += conf->udata_size;
  Z_LOG_DEBUG("global-context memory waste %u/%u (%u)",
              gctx_size, z_align_up(gctx_size, __PAGE_ALIGN),
              z_align_up(gctx_size, __PAGE_ALIGN) - gctx_size);
  gctx_size += conf->ncores * eloop_size;
  gctx_size  = z_align_up(gctx_size, __PAGE_ALIGN);

  Z_LOG_DEBUG("z_global_context_t %u", (uint32_t)(sizeof(z_global_context_t) - 1));
  Z_LOG_DEBUG("z_event_loop_t     %u", (uint32_t)sizeof(z_event_loop_t));

  /* Allocate the global-context */
  gctx = Z_CAST(z_global_context_t, z_raw_alloc(gctx_size));
  if (Z_UNLIKELY(gctx == NULL)) {
    Z_LOG_ERRNO_ERROR("Unable to allocate global-context, size=%u eloop=%u",
                      gctx_size, eloop_size);
    return(-1);
  }

  do {
    char buf0[16], buf1[16];
    Z_LOG_DEBUG("z_global_context %s (%u) - eloop %s (%u) - ncores %u",
                z_human_size(buf0, sizeof(buf0), gctx_size), gctx_size,
                z_human_size(buf1, sizeof(buf1), eloop_size), eloop_size,
                conf->ncores);
  } while (0);

  /* Initialize the global-context */
  gctx->eloop_data = (conf->ncores * sizeof(z_event_loop_t));
  gctx->eloop_size = eloop_size;
  gctx->user_data  = gctx->eloop_data + (conf->ncores * eloop_size);
  gctx->ctx_size   = gctx_size;

  gctx->is_running = 0;
  gctx->balance    = 0;
  memcpy(&(gctx->conf), conf, sizeof(z_global_conf_t));

  Z_LOG_DEBUG("eloop_data %u", gctx->eloop_data);
  Z_LOG_DEBUG("eloop_size %u", gctx->eloop_size);
  Z_LOG_DEBUG("user_data  %u", gctx->user_data);

  /* Initialize event-loops */
  for (i = 0; i < conf->ncores; ++i) {
    Z_LOG_DEBUG("Initialize eloop %u", i);
    if (z_event_loop_open(__ctx_eloop(gctx, i), gctx, i)) {
      while (i--) {
        z_event_loop_close(__ctx_eloop(gctx, i));
      }
      z_raw_free(gctx, gctx->ctx_size);
      return(1);
    }
  }

  __global_ctx = gctx;
  return(0);
}

int z_global_context_open_default (uint32_t ncores) {
  z_global_conf_t conf;

  /* Default Settings */
  memset(&conf, 0, sizeof(z_global_conf_t));
  conf.ncores = ncores;

  /* Memory Pool */
  conf.mmpool_base_size = (1 << 20);
  conf.mmpool_block_min = (1 << 4);
  conf.mmpoll_block_max = (1 << 9);
  conf.mmpool_page_size = (1 << 20);

  /* Events */
  conf.events_ring_size = (1 << 20);

  return z_global_context_open(&conf);
}

void z_global_context_close (void) {
  uint32_t i;
  Z_ASSERT_BUG(__global_ctx != NULL, "no global-context initialized");
  for (i = 0; i < __global_ctx->conf.ncores; ++i) {
    z_event_loop_close(__global_eloop(i));
  }
  z_debug_close();
  z_raw_free(__global_ctx, __global_ctx->ctx_size);
  __global_ctx = NULL;
}

int z_global_context_poll (int detached, int *is_running) {
  Z_ASSERT_BUG(__global_ctx != NULL, "no global-context initialized");
  Z_ASSERT(is_running != NULL, "is_running must be a non NULL pointer");
  __global_ctx->is_running = is_running;

  /* eloop[1:] are running on other threads */
  if (detached || __global_ctx->conf.ncores > 1) {
    uint32_t i;
    for (i = !detached; i < __global_ctx->conf.ncores; ++i) {
      Z_LOG_DEBUG("Start eloop %u", i);
      if (z_event_loop_start(__global_eloop(i), 1)) {
        while (i--) {
          z_event_loop_stop(__global_eloop(i));
        }
        return(-1);
      }
    }

    if (detached)
      return(0);
  }

  /* engine[0] is running on the main thread */
  Z_LOG_DEBUG("Start eloop 0 (main-loop)");
  if (z_event_loop_start(__global_eloop(0), 0)) {
    uint32_t i;
    for (i = 1; i < __global_ctx->conf.ncores; ++i) {
      z_event_loop_stop(__global_eloop(i));
    }
  }

  /* wait */
  do {
    uint32_t i;
    for (i = 1; i < __global_ctx->conf.ncores; ++i) {
      z_event_loop_wait(__global_eloop(i));
    }
  } while (0);
  return(0);
}

void z_global_context_stop (void) {
  uint32_t i;
  Z_ASSERT_BUG(__global_ctx != NULL, "no global-context initialized");
  for (i = 0; i < __global_ctx->conf.ncores; ++i) {
    z_event_loop_stop(__global_eloop(i));
  }
}

void z_global_context_wait (void) {
  uint32_t i;
  Z_ASSERT_BUG(__global_ctx != NULL, "no global-context initialized");
  for (i = 0; i < __global_ctx->conf.ncores; ++i) {
    z_event_loop_wait(__global_eloop(i));
  }
}

/* ============================================================================
 *  PUBLIC global object getters
 */
void *z_global_udata (void) {
  return(__global_ctx->data + __global_ctx->user_data);
}

int *z_global_is_running (void) {
  return(__global_ctx->is_running);
}

uint32_t z_global_balance (void) {
  return z_atomic_inc(&(__global_ctx->balance)) % __global_ctx->conf.ncores;
}

z_memory_t *z_global_memory (void) {
  return(&(__current_event_loop()->memory));
}

z_memory_t *z_global_memory_at (uint32_t core) {
  return(&(__global_eloop(core)->memory));
}

z_iopoll_engine_t *z_global_iopoll (void) {
  return(&(__current_event_loop()->iopoll));
}

z_iopoll_engine_t *z_global_iopoll_at (uint32_t core) {
  return(&(__global_eloop(core)->iopoll));
}

const z_iopoll_stats_t *z_global_iopoll_stats (void) {
  return(&(__current_event_loop()->iopoll.stats));
}

const z_iopoll_stats_t *z_global_iopoll_stats_at (uint32_t core) {
  return(&(__global_eloop(core)->iopoll.stats));
}
