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
#include <zcl/atomic.h>
#include <zcl/global.h>
#include <zcl/system.h>
#include <zcl/debug.h>
#include <zcl/time.h>

#define __CPU_CTX(x)                  Z_CAST(struct cpu_ctx, cpu_ctx)

struct cpu_ctx {
  z_task_rq_rr_t rq;
  uint8_t __pad1[Z_CACHELINE_PAD(sizeof(z_task_rq_rr_t))];

  z_task_rq_stats_t rq_stats;

  z_memory_t memory;

  uint32_t  wait_lock;
  uint32_t  notify;
  z_mutex_t mutex;
  z_wait_cond_t task_ready;
  uint8_t  __pad0[Z_CACHELINE_PAD_FIELDS(uint32_t, uint32_t, z_mutex_t, z_wait_cond_t)];

  void *user_data;
  z_thread_t thread;
  uint8_t __pad2[Z_CACHELINE_PAD_FIELDS(void *, z_thread_t)];
};

struct global_ctx {
  z_mutex_t     mutex;
  z_wait_cond_t no_active_threads;
  uint32_t      waiting_threads;
  uint32_t      total_threads;
  uint8_t       is_running;
  uint8_t       __pad[3];
  uint32_t      balancer;
  uint8_t __pad0[Z_CACHELINE_PAD_FIELDS(z_mutex_t, z_wait_cond_t, uint64_t, uint64_t)];

  z_allocator_t * allocator;
  void *          user_data;
  uint64_t        start_time;
  uint8_t __pad1[Z_CACHELINE_PAD_FIELDS(z_allocator_t *, void *, uint64_t)];

  struct cpu_ctx  cpus[1];
};

/* ============================================================================
 *  PRIVATE global-context instance
 */
static struct global_ctx *__global_ctx = NULL;

/* ============================================================================
 *  PRIVATE cpu-context methods
 */
#define __cpu_ctx_id(cpu_ctx)   ((int)(__CPU_CTX(cpu_ctx) - __global_ctx->cpus))

#define __CPU_WAIT_TIMEOUT      Z_TIME_SEC(10)
#define __CPU_STEAL_TASK        0

#if __THREAD_POOL_STEAL_TASK
static z_vtask_t *__cpu_steal_task (struct cpu_ctx *cpu_ctx) {
  unsigned int count = __global_ctx->total_threads - 1;
  unsigned int index = __cpu_ctx_id(cpu_ctx);
  unsigned int mask;
  z_vtask_t *vtask;

  mask = count - 1;
  do {
    struct cpu_ctx *other_ctx;
    index = (index << 2) + index + 1;
    other_ctx = &(__global_ctx->cpus[index & mask]);
    if (z_busy_try_lock(&(other_ctx->wait_lock))) {
      vtask = z_task_rq_fetch(&(other_ctx->rq.rq));
      z_busy_unlock(&(other_ctx->wait_lock));
    }
  } while (vtask == NULL && --count);
  return(vtask);
}
#endif

static void __cpu_wait (struct cpu_ctx *cpu_ctx) {
  int i;

  z_busy_lock(&(cpu_ctx->wait_lock));

  /* atomic spin */
  for (i = 0; i < 500; ++i) {
    if (z_atomic_cas(&(cpu_ctx->notify), 1, 0)) {
      z_busy_unlock(&(cpu_ctx->wait_lock));
      return;
    }
    z_thread_yield();
  }

  /* yield a couple of times to avoid locks, hoping that someone will add a task */
  for (i = 0; i < 100; ++i) {
    if (z_atomic_cas(&(cpu_ctx->notify), 1, 0)) {
      z_busy_unlock(&(cpu_ctx->wait_lock));
      return;
    }
    z_time_usleep(250);
  }

  if (cpu_ctx->notify == 0 && __global_ctx->is_running) {
    z_mutex_lock(&(cpu_ctx->mutex));
    if (cpu_ctx->notify == 0 && __global_ctx->is_running) {
      if (z_atomic_inc(&(__global_ctx->waiting_threads)) == __global_ctx->total_threads) {
        z_wait_cond_broadcast(&(__global_ctx->no_active_threads));
      }
      z_wait_cond_wait(&(cpu_ctx->task_ready), &(cpu_ctx->mutex), __CPU_WAIT_TIMEOUT);
      z_atomic_dec(&(__global_ctx->waiting_threads));
    }
    z_mutex_unlock(&(cpu_ctx->mutex));
  }

  z_busy_unlock(&(cpu_ctx->wait_lock));
}

static void *__cpu_ctx_loop (void *cpu_ctx) {
  uint64_t now;

  now = z_time_micros();
  while (__global_ctx->is_running) {
    uint64_t stime = now;
    z_vtask_t *vtask;

    __CPU_CTX(cpu_ctx)->notify = 0;
    vtask = z_task_rq_fetch(&(__CPU_CTX(cpu_ctx)->rq.rq));
#if __THREAD_POOL_STEAL_TASK
    if (Z_UNLIKELY(vtask == NULL)) {
      /* Try stealing from another queue */
      vtask = __cpu_steal_task(__CPU_CTX(cpu_ctx));
    }
#endif

    now = z_time_micros();
    z_histogram_add(&(__CPU_CTX(cpu_ctx)->rq_stats.rq_time), now - stime);

    stime = now;
    if (Z_LIKELY(vtask != NULL)) {
      z_vtask_exec(vtask);
      now = z_time_micros();
      z_histogram_add(&(__CPU_CTX(cpu_ctx)->rq_stats.task_exec), now - stime);
    } else {
      __cpu_wait(__CPU_CTX(cpu_ctx));
      now = z_time_micros();
      z_histogram_add(&(__CPU_CTX(cpu_ctx)->rq_stats.rq_wait), now - stime);
    }
  }

  /* Shutdown worker */
  z_mutex_lock(&(__global_ctx->mutex));
  if (z_atomic_inc(&(__global_ctx->waiting_threads)) == __global_ctx->total_threads) {
    z_wait_cond_broadcast(&(__global_ctx->no_active_threads));
  }
  z_mutex_unlock(&(__global_ctx->mutex));
  return(NULL);
}

static void __cpu_ctx_dump (struct cpu_ctx *cpu_ctx) {
  uint64_t nevents;

  nevents = z_histogram_nevents(&(cpu_ctx->rq_stats.task_exec));
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

  fprintf(stdout, "RQ-Time ");
  z_histogram_dump(&(cpu_ctx->rq_stats.rq_time), stdout, z_human_dtime);
  fprintf(stdout, "\n");

  fprintf(stdout, "Task-Exec ");
  z_histogram_dump(&(cpu_ctx->rq_stats.task_exec), stdout, z_human_dtime);
  fprintf(stdout, "\n");

  fprintf(stdout, "RQ-Wait ");
  z_histogram_dump(&(cpu_ctx->rq_stats.rq_wait), stdout, z_human_dtime);
  fprintf(stdout, "\n");
}

static int __cpu_ctx_open (struct cpu_ctx *cpu_ctx, int cpu_id) {
  /* Initialize Memory */
  z_memory_open(&(cpu_ctx->memory), __global_ctx->allocator);

  /* Initialize Task Histogram */
  z_task_rq_stats_init(&(cpu_ctx->rq_stats));

  /* Initialize RQ */
  z_task_rq_open(&(cpu_ctx->rq.rq), &z_task_rq_rr, 0);

  /* Initialize User Data */
  cpu_ctx->user_data = NULL;

  /* Initialize wait spin-lock */
  z_busy_init(&(cpu_ctx->wait_lock));
  cpu_ctx->notify = 0;

  /* Initialize RQ-Wait mutex */
  if (z_mutex_alloc(&(cpu_ctx->mutex))) {
    Z_LOG_FATAL("unable to initialize the cpu-ctx mutex.");
    return(1);
  }

  /* Initialize RQ-Wait condition */
  if (z_wait_cond_alloc(&(cpu_ctx->task_ready))) {
    Z_LOG_FATAL("unable to initialize the cpu-ctx rq wait condition.");
    z_mutex_free(&(cpu_ctx->mutex));
    return(2);
  }

  /* Create the cpu context thread */
  if (z_thread_start(&(cpu_ctx->thread), __cpu_ctx_loop, cpu_ctx)) {
    Z_LOG_FATAL("unable to initialize the thread for cpu %d.", cpu_id);
    z_wait_cond_free(&(cpu_ctx->task_ready));
    z_mutex_free(&(cpu_ctx->mutex));
    return(3);
  }

  /* Bind the cpu context to the specified core */
  z_thread_bind_to_core(&(cpu_ctx->thread), cpu_id);
  return(0);
}

static void __cpu_ctx_close (struct cpu_ctx *cpu_ctx) {
  z_thread_join(&(cpu_ctx->thread));
  z_wait_cond_free(&(cpu_ctx->task_ready));
  z_mutex_free(&(cpu_ctx->mutex));
  z_task_rq_close(&(cpu_ctx->rq.rq));
  z_memory_close(&(cpu_ctx->memory));
}

/* ============================================================================
 *  PRIVATE cpu-context thread-local lookups
 */
static __thread struct cpu_ctx *__local_cpu_ctx = NULL;
static struct cpu_ctx *__set_local_cpu_ctx (void) {
  unsigned int ncpus = __global_ctx->total_threads;
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
    ncpus = __global_ctx->total_threads;
    cpu_ctx = &(__global_ctx->cpus[core % ncpus]);
  }

  Z_LOG_DEBUG("Lookup Local CPU-ctx for thread %ld (CPU ID %ld Is-Task-Exec %d)",
              (long)tid, cpu_ctx - __global_ctx->cpus, !not_found);
  __local_cpu_ctx = cpu_ctx;
  return(cpu_ctx);
}

#define __current_cpu_ctx()                       \
  (Z_LIKELY(__local_cpu_ctx != NULL) ?            \
    __local_cpu_ctx : __set_local_cpu_ctx())

/* ============================================================================
 *  PRIVATE global-ctx methods
 */
static int __thread_pool_open (int ncpus) {
  if (z_mutex_alloc(&(__global_ctx->mutex))) {
    Z_LOG_FATAL("unable to initialize the thread-pool mutex.");
    return(1);
  }

  if (z_wait_cond_alloc(&(__global_ctx->no_active_threads))) {
    Z_LOG_FATAL("unable to initialize the thread-pool wait condition");
    z_mutex_free(&(__global_ctx->mutex));
    return(1);
  }

  __global_ctx->waiting_threads = 0;
  __global_ctx->total_threads = ncpus;
  __global_ctx->is_running = 1;
  __global_ctx->balancer = 0;
  return(0);
}

static void __thread_pool_close (void) {
  z_wait_cond_free(&(__global_ctx->no_active_threads));
  z_mutex_free(&(__global_ctx->mutex));
}

static void __thread_pool_wait (void) {
  uint32_t waiting_threads;
  z_mutex_lock(&(__global_ctx->mutex));
  waiting_threads = z_atomic_and_and_fetch(&(__global_ctx->waiting_threads), 0xffffffff);
  if (waiting_threads < __global_ctx->total_threads) {
    z_wait_cond_wait(&(__global_ctx->no_active_threads), &(__global_ctx->mutex), 0);
  }
  z_mutex_unlock(&(__global_ctx->mutex));
}

/* ============================================================================
 *  PUBLIC global-ctx methods
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
  __global_ctx->start_time = z_time_micros();

  Z_LOG_DEBUG("Initialize global-context thread-pool size %u", ncpus);
  if (__thread_pool_open(ncpus)) {
    Z_LOG_FATAL("unable to initialize the global context thread-pool.");
    z_allocator_free(allocator, __global_ctx);
    __global_ctx = NULL;
    return(2);
  }

  /* Initialize cpu context */
  cpu_ctx = __global_ctx->cpus;
  while (ncpus--) {
    Z_LOG_DEBUG("Initialize global-context cpu-ctx %d", __cpu_ctx_id(cpu_ctx));
    if (__cpu_ctx_open(cpu_ctx++, ncpus)) {
      while (++ncpus < __global_ctx->total_threads)
        __cpu_ctx_close(--cpu_ctx);

      __thread_pool_close();
      z_allocator_free(allocator, __global_ctx);
      __global_ctx = NULL;
      return(5);
    }
  }

  /* Wait thread pool initialization */
  Z_LOG_DEBUG("Waiting for global-context thread-pool initialization");
  __thread_pool_wait();
  Z_LOG_DEBUG("Completed initialization of global-context");
  return(0);
}

void z_global_context_close (void) {
  int i, size = __global_ctx->total_threads;

  /* Send the stop signal */
  z_global_context_stop();

  /* clear queue */
  for (i = 0; i < size; ++i) {
    __cpu_ctx_close(&(__global_ctx->cpus[i]));
  }
  __thread_pool_close();

  /* Dump cpu-ctx */
  for (i = 0; i < size; ++i) {
    __cpu_ctx_dump(&(__global_ctx->cpus[i]));
  }

  /* Release the global-ctx */
  z_allocator_free(__global_ctx->allocator, __global_ctx);
  __global_ctx = NULL;

  /* Uninitialize Debug env */
  z_debug_close();
}

void z_global_context_stop (void) {
  int i;

  __global_ctx->is_running = 0;
  for (i = 0; i < __global_ctx->total_threads; ++i) {
    struct cpu_ctx *cpu_ctx = &(__global_ctx->cpus[i]);
    z_mutex_lock(&(cpu_ctx->mutex));
    z_atomic_cas(&(cpu_ctx->notify), 0, 1);
    z_wait_cond_broadcast(&(cpu_ctx->task_ready));
    z_mutex_unlock(&(cpu_ctx->mutex));
  }

  z_global_context_wait();
}

void z_global_context_wait (void) {
  __thread_pool_wait();
}

/* ============================================================================
 *  PUBLIC global cpu-ctx user data getters/setters
 */
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

z_task_rq_t *z_global_cpu_rq (int core) {
  return(&(__global_ctx->cpus[core].rq.rq));
}

/* ============================================================================
 *  PUBLIC global object getters
 */
z_task_rq_t *z_global_rq (void) {
  unsigned int core;
  core = z_atomic_inc(&(__global_ctx->balancer)) % __global_ctx->total_threads;
  return(&(__global_ctx->cpus[core].rq.rq));
}

z_memory_t *z_global_memory (void) {
  return(&(__current_cpu_ctx()->memory));
}

z_allocator_t *z_global_allocator (void) {
  return(__global_ctx->allocator);
}

const uint8_t *z_global_is_running (void) {
  return(&(__global_ctx->is_running));
}

unsigned int z_global_cpu_count (void) {
  return(__global_ctx->total_threads);
}

void *z_global_user_data (void) {
  return(__global_ctx->user_data);
}

uint64_t z_global_uptime (void) {
  return(z_time_micros() - __global_ctx->start_time);
}

/* ============================================================================
 *  PUBLIC global metrics getters
 */
const z_histogram_t *z_global_histo_memory (int core) {
  return(&(__global_ctx->cpus[core].memory.histo));
}

const z_task_rq_stats_t *z_global_rq_stats (int core) {
  return(&(__global_ctx->cpus[core].rq_stats));
}

/* ============================================================================
 *  PUBLIC new task notifications
 */
void z_global_new_task_signal (z_task_rq_t *rq) {
  struct cpu_ctx *cpu_ctx;

  Z_ASSERT(rq->vtask.parent == NULL, "Must be a global-rq");
  cpu_ctx = z_container_of(rq, struct cpu_ctx, rq);
  Z_ASSERT(&(cpu_ctx->rq.rq) == rq, "Wrong RQ");

  if (z_busy_try_lock(&(cpu_ctx->wait_lock))) {
    z_atomic_cas(&(cpu_ctx->notify), 0, 1);
    z_busy_unlock(&(cpu_ctx->wait_lock));
  } else {
    z_mutex_lock(&(cpu_ctx->mutex));
    z_atomic_cas(&(cpu_ctx->notify), 0, 1);
    z_wait_cond_signal(&(cpu_ctx->task_ready));
    z_mutex_unlock(&(cpu_ctx->mutex));
  }
}

void z_global_new_tasks_broadcast (void) {
  int size = __global_ctx->total_threads;
  struct cpu_ctx *cpu_ctx;

  for (cpu_ctx = __global_ctx->cpus; size--; ++cpu_ctx) {
    z_mutex_lock(&(cpu_ctx->mutex));
    z_atomic_cas(&(cpu_ctx->notify), 0, 1);
    z_wait_cond_broadcast(&(cpu_ctx->task_ready));
    z_mutex_unlock(&(cpu_ctx->mutex));
  }
}
