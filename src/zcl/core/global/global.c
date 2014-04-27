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

#include <zcl/threadpool.h>
#include <zcl/histogram.h>
#include <zcl/task-rq.h>
#include <zcl/global.h>
#include <zcl/system.h>
#include <zcl/debug.h>
#include <zcl/time.h>

#define __CPU_CTX(x)                  Z_CAST(struct cpu_ctx, cpu_ctx)

struct cpu_ctx {
  z_histogram_t rq_time;
  uint64_t rq_time_events[18];

  z_histogram_t task_vtime;
  uint64_t task_vtime_events[18];

  z_memory_t memory;

  void *user_data;
  z_thread_t thread;
  uint8_t pad1[Z_CACHELINE_PAD(sizeof(z_thread_t) + sizeof(void *))];
};

struct global_ctx {
  z_task_rq_rr_t rq;
  uint8_t pad1[Z_CACHELINE_PAD(sizeof(z_task_rq_rr_t))];

  z_thread_pool_t thread_pool;
  uint8_t pad2[Z_CACHELINE_PAD(sizeof(z_thread_pool_t))];

  z_allocator_t * allocator;
  void *          user_data;
  uint8_t pad3[Z_CACHELINE_PAD(sizeof(z_allocator_t *) + sizeof(void *))];

  struct cpu_ctx  cpus[1];
};

/* ============================================================================
 *  PRIVATE task-vtime histogram bounds
 */
#define __STATS_HISTO_NBOUNDS       z_fix_array_size(__STATS_HISTO_BOUNDS)
static const uint64_t __STATS_HISTO_BOUNDS[18] = {
  Z_TIME_USEC(5),   Z_TIME_USEC(10),  Z_TIME_USEC(25),  Z_TIME_USEC(75),
  Z_TIME_USEC(250), Z_TIME_USEC(500), Z_TIME_MSEC(1),   Z_TIME_MSEC(5),
  Z_TIME_MSEC(10),  Z_TIME_MSEC(25),  Z_TIME_MSEC(75),  Z_TIME_MSEC(250),
  Z_TIME_MSEC(500), Z_TIME_MSEC(750), Z_TIME_SEC(1),    Z_TIME_SEC(2),
  Z_TIME_SEC(5),    0xffffffffffffffffll,
};

/* ============================================================================
 *  PRIVATE global-context instance
 */
static struct global_ctx *__global_ctx = NULL;

/* ============================================================================
 *  PRIVATE cpu-context methods
 */
#define __cpu_ctx_id(cpu_ctx)       (__CPU_CTX(cpu_ctx) - __global_ctx->cpus)

static void *__cpu_ctx_loop (void *cpu_ctx) {
  uint64_t __stime = z_time_micros();
  uint64_t unlock_no_task = 0;
  uint64_t nreqests = 0;
  int is_running = 1;
  uint64_t now;

  now = z_time_micros();
  while (is_running) {
    z_vtask_t *vtask;
    uint64_t stime;

    stime = now;
    vtask = z_task_rq_fetch(&(__global_ctx->rq.rq));
    now = z_time_micros();
    z_histogram_add(&(__CPU_CTX(cpu_ctx)->rq_time), now - stime);

    if (Z_LIKELY(vtask != NULL)) {
      uint64_t vtime;

      stime = now;
      z_vtask_exec(vtask);
      nreqests += 1;
      now = z_time_micros();

      vtime = (now - stime);
      //vtask->vtime += vtime;
      z_histogram_add(&(__CPU_CTX(cpu_ctx)->task_vtime), vtime);
      is_running = __global_ctx->thread_pool.is_running;
    } else {
      unlock_no_task += 1;
      is_running = z_thread_pool_worker_wait(&(__global_ctx->thread_pool));
    }
  }

  Z_LOG_TRACE("CPU %ld %7"PRIu64" %.3freq/sec - unlock-timeout=%"PRIu64,
          __cpu_ctx_id(cpu_ctx), nreqests,
          (float)nreqests / ((z_time_micros() - __stime) / 1000000.0),
          unlock_no_task);

  z_thread_pool_worker_close(&(__global_ctx->thread_pool));
  return(NULL);
}

static void __cpu_ctx_dump (struct cpu_ctx *cpu_ctx) {
  char buf0[16], buf1[16], buf2[16];
  uint64_t nevents;

  nevents = z_histogram_nevents(&(cpu_ctx->task_vtime));
  if (nevents == 0) {
    return;
  }

  fprintf(stdout, "Memory Stats CPU %d\n", (int)(cpu_ctx - __global_ctx->cpus));
  fprintf(stdout, "==================================\n");
  z_memory_stats_dump(&(cpu_ctx->memory), stdout);
  fprintf(stdout, "\n");

  fprintf(stdout, "Task Stats CPU %d\n", (int)(cpu_ctx - __global_ctx->cpus));
  fprintf(stdout, "==================================\n");
  fprintf(stdout, "Task events:      %"PRIu64"\n", nevents);
  fprintf(stdout, "avg Task vtime:   %s (%s-%s)\n",
        z_human_time(buf0, sizeof(buf0), z_histogram_average(&(cpu_ctx->task_vtime))),
        z_human_time(buf1, sizeof(buf1), z_histogram_percentile(&(cpu_ctx->task_vtime), 0)),
        z_human_time(buf2, sizeof(buf2), z_histogram_percentile(&(cpu_ctx->task_vtime), 99.9999)));

  fprintf(stdout, "RQ-Time ");
  z_histogram_dump(&(cpu_ctx->rq_time), stdout, z_human_time);
  fprintf(stdout, "\n");

  fprintf(stdout, "VTime ");
  z_histogram_dump(&(cpu_ctx->task_vtime), stdout, z_human_time);
  fprintf(stdout, "\n");
}

static int __cpu_ctx_open (struct cpu_ctx *cpu_ctx, int cpu_id) {
  /* Initialize Memory */
  z_memory_open(&(cpu_ctx->memory), __global_ctx->allocator);

  /* Initialize Task Histogram */
  z_histogram_init(&(cpu_ctx->task_vtime),  __STATS_HISTO_BOUNDS,
                   cpu_ctx->task_vtime_events,  __STATS_HISTO_NBOUNDS);
  z_histogram_init(&(cpu_ctx->rq_time),  __STATS_HISTO_BOUNDS,
                   cpu_ctx->rq_time_events,  __STATS_HISTO_NBOUNDS);

  cpu_ctx->user_data = NULL;

  /* Create the cpu context thread */
  if (z_thread_start(&(cpu_ctx->thread), __cpu_ctx_loop, cpu_ctx)) {
    Z_LOG_FATAL("unable to initialize the thread for cpu %d.", cpu_id);
    return(1);
  }

  /* Bind the cpu context to the specified core */
  z_thread_bind_to_core(&(cpu_ctx->thread), cpu_id);
  return(0);
}

static void __cpu_ctx_close (struct cpu_ctx *cpu_ctx) {
  z_thread_join(&(cpu_ctx->thread));
  z_memory_close(&(cpu_ctx->memory));
}

/* ============================================================================
 *  PRIVATE cpu-context thread-local lookups
 */
static __thread struct cpu_ctx *__local_cpu_ctx = NULL;
static struct cpu_ctx *__set_local_cpu_ctx (void) {
  int ncpus = __global_ctx->thread_pool.total_threads;
  struct cpu_ctx *cpu_ctx = __global_ctx->cpus;
  int not_found = 1;
  z_thread_t tid;

  z_thread_self(&tid);
  while (--ncpus && (not_found = (cpu_ctx->thread != tid))) {
    ++cpu_ctx;
  }

  if (not_found) {
    int core = 0;
    z_thread_get_core(&tid, &core);
    ncpus = __global_ctx->thread_pool.total_threads;
    cpu_ctx = &(__global_ctx->cpus[core % ncpus]);
  }

  fprintf(stderr, "Lookup Local CPU-ctx for thread %ld (CPU ID %ld Is-Task %d)\n",
                  (long)tid, cpu_ctx - __global_ctx->cpus, !not_found);
  __local_cpu_ctx = cpu_ctx;
  return(cpu_ctx);
}

#define __current_cpu_ctx()                       \
  (Z_LIKELY(__local_cpu_ctx != NULL) ?            \
    __local_cpu_ctx : __set_local_cpu_ctx())

/* ============================================================================
 *  PUBLIC
 */
int z_global_context_open (z_allocator_t *allocator, void *user_data) {
  char buf0[16], buf1[16], buf2[16];
  struct cpu_ctx *cpu_ctx;
  size_t mmsize;
  int ncpus;

  if (__global_ctx != NULL) {
    Z_LOG_WARN("global context exists already!");
    return(0);
  }

  /* Initialize Debug env */
  z_debug_open();

  /* Allocate the global-ctx object */
  ncpus = z_align_up(z_system_processors(), 2);
  mmsize = sizeof(struct global_ctx) + ((ncpus - 1) * sizeof(struct cpu_ctx));
  __global_ctx = z_allocator_alloc(allocator, struct global_ctx, mmsize);
  if (__global_ctx == NULL) {
    Z_LOG_FATAL("unable to allocate the global context.");
    return(1);
  }

  Z_LOG_DEBUG("z_global_context %s (mmsize %s) - ncpus %d - cpu_ctx %s",
              z_human_size(buf0, sizeof(buf0), sizeof(struct global_ctx)),
              z_human_size(buf1, sizeof(buf1), mmsize), ncpus,
              z_human_size(buf2, sizeof(buf2), sizeof(struct cpu_ctx)));

  /* Initialize the global context */
  __global_ctx->allocator = allocator;
  __global_ctx->user_data = user_data;

  Z_LOG_DEBUG("Initialize global-context run-queue");
  z_task_rq_open(&(__global_ctx->rq.rq), &z_task_rq_rr, 0);

  Z_LOG_DEBUG("Initialize global-context thread-pool");
  if (z_thread_pool_open(&(__global_ctx->thread_pool), ncpus)) {
    Z_LOG_FATAL("unable to initialize the global context thread-pool.");
    z_allocator_free(allocator, __global_ctx);
    __global_ctx = NULL;
    return(2);
  }

  /* Initialize cpu context */
  cpu_ctx = __global_ctx->cpus;
  while (ncpus--) {
    Z_LOG_DEBUG("Initialize global-context cpu-ctx %d", ncpus);
    if (__cpu_ctx_open(cpu_ctx++, ncpus)) {
      while (++ncpus < __global_ctx->thread_pool.total_threads)
        __cpu_ctx_close(--cpu_ctx);

      z_thread_pool_close(&(__global_ctx->thread_pool));
      z_allocator_free(allocator, __global_ctx);
      __global_ctx = NULL;
      return(5);
    }
  }

  /* Wait thread pool initialization */
  Z_LOG_DEBUG("Waiting for global-context thread-pool initialization");
  z_thread_pool_wait(&(__global_ctx->thread_pool));
  Z_LOG_DEBUG("Completed initialization of global-context");
  return(0);
}

void z_global_context_close (void) {
  int i, total_threads = __global_ctx->thread_pool.total_threads;

  /* clear queue */
  z_thread_pool_stop(&(__global_ctx->thread_pool));
  for (i = 0; i < total_threads; ++i) {
    __cpu_ctx_close(&(__global_ctx->cpus[i]));
  }
  z_thread_pool_close(&(__global_ctx->thread_pool));


  /* Dump cpu-ctx */
  for (i = 0; i < total_threads; ++i) {
    __cpu_ctx_dump(&(__global_ctx->cpus[i]));
  }

  z_task_rq_close(&(__global_ctx->rq.rq));

  z_allocator_free(__global_ctx->allocator, __global_ctx);
  __global_ctx = NULL;

  /* Uninitialize Debug env */
  z_debug_close();
}

void z_global_context_stop (void) {
  z_thread_pool_stop(&(__global_ctx->thread_pool));
}

void z_global_context_wait (void) {
  z_thread_pool_wait(&(__global_ctx->thread_pool));
}

void *z_global_cpu_ctx (void) {
  return(__current_cpu_ctx()->user_data);
}

void z_global_set_cpu_ctx (void *udata) {
  __current_cpu_ctx()->user_data = udata;
}

void *z_global_cpu_ctx_id (int core) {
  return(__global_ctx->cpus[core].user_data);
}

void z_global_set_cpu_ctx_id (int core, void *udata) {
  __global_ctx->cpus[core].user_data = udata;
}

z_task_rq_t *z_global_rq (void) {
  return(&(__global_ctx->rq.rq));
}

z_memory_t *z_global_memory (void) {
  return(&(__current_cpu_ctx()->memory));
}

void z_global_new_task_signal (void) {
  z_wait_cond_signal(&(__global_ctx->thread_pool.task_ready));
}

void z_global_new_tasks_broadcast (void) {
  z_wait_cond_broadcast(&(__global_ctx->thread_pool.task_ready));
}

void z_global_new_tasks_signal (int ntasks) {
  if (ntasks > 1) {
    z_wait_cond_broadcast(&(__global_ctx->thread_pool.task_ready));
  } else {
    z_wait_cond_signal(&(__global_ctx->thread_pool.task_ready));
  }
}
