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

#include <zcl/threading.h>
#include <zcl/locking.h>
#include <zcl/global.h>
#include <zcl/system.h>
#include <zcl/debug.h>
#include <zcl/time.h>

#define __NUM_WORK_QUEUES         (1)

struct cpu_ctx {
  z_thread_t thread;
  uint8_t pad0[z_align_up(sizeof(z_thread_t), 128) - sizeof(z_thread_t)];

  z_memory_t memory;
  uint8_t pad[z_align_up(sizeof(z_memory_t), 128) - sizeof(z_memory_t)];
};

struct z_global_context {
  uint64_t req_id;
  z_task_tree_t  pending[__NUM_WORK_QUEUES];
  z_task_queue_t queue[__NUM_WORK_QUEUES];

  z_thread_pool_t thread_pool;

  z_allocator_t *allocator;
  void *         user_data;

  struct cpu_ctx cpus[1];
};

/* ============================================================================
 *  PRIVATE global-context instance
 */
static z_global_context_t *__global_ctx = NULL;

/* ============================================================================
 *  PRIVATE cpu-context methods
 */
static void *__cpu_ctx_fetch_task (void *cpu_ctx) {
  z_task_t *task;
#if __NUM_WORK_QUEUES > 1
  int count;

  count = __NUM_WORK_QUEUES;
  while (count--) {
    if ((task = z_task_tree_pop(&(__global_ctx->pending[count]))) != NULL)
      return(task);
  }

  count = __NUM_WORK_QUEUES;
  while (count--) {
    if ((task = z_task_queue_pop(&(__global_ctx->queue[count]))) != NULL)
      return(task);
  }
#else
  if ((task = z_task_tree_pop(&(__global_ctx->pending[0]))) != NULL)
    return(task);

  if ((task = z_task_queue_pop(&(__global_ctx->queue[0]))) != NULL)
    return(task);
#endif
  return(NULL);
}

static void *__cpu_ctx_loop (void *cpu_ctx) {
  int is_running = 1;
  while (is_running) {
    void *task = z_thread_pool_ctx_fetch(&(__global_ctx->thread_pool),
                                         __cpu_ctx_fetch_task, cpu_ctx,
                                         &is_running);
    if (Z_LIKELY(task != NULL)) {
      z_task_exec(task);
    }
  }

  z_thread_pool_ctx_close(&(__global_ctx->thread_pool));
  return(NULL);
}

static int __cpu_ctx_open (struct cpu_ctx *cpu_ctx, int cpu_id) {
  /* Create the cpu context thread */
  if (z_thread_start(&(cpu_ctx->thread), __cpu_ctx_loop, cpu_ctx)) {
    Z_LOG_FATAL("unable to initialize the thread for cpu %d.", cpu_id);
    return(1);
  }

  /* Bind the cpu context to the specified core */
  z_thread_bind_to_core(&(cpu_ctx->thread), cpu_id);

  /* Initialize Memory */
  z_memory_open(&(cpu_ctx->memory), __global_ctx->allocator);

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
  z_thread_t tid;

  z_thread_self(&tid);
  while (--ncpus && cpu_ctx->thread != tid) {
    ++cpu_ctx;
  }
  fprintf(stderr, "Lookup Local CPU-ctx for thread %ld (CPU ID %ld)\n",
                  (long)tid, cpu_ctx - __global_ctx->cpus);
  __local_cpu_ctx = cpu_ctx;
  return(cpu_ctx);
}

#define __current_cpu_ctx()                                                    \
  (Z_LIKELY(__local_cpu_ctx != NULL) ? __local_cpu_ctx : __set_local_cpu_ctx())

#define __current_cpu_ctx_id()    (__current_cpu_ctx() - __global_ctx->cpus)
#define __current_queue_slot()    (__current_cpu_ctx_id() & (__NUM_WORK_QUEUES - 1))

/* ============================================================================
 *  PUBLIC global-context methods
 */
int z_global_context_open (z_allocator_t *allocator, void *user_data) {
  struct cpu_ctx *cpu_ctx;
  unsigned int mmsize;
  int i, ncpus;

  if (__global_ctx != NULL) {
    Z_LOG_WARN("global context exists already!");
    return(0);
  }

  /* Initialize Debug env */
  z_debug_open();

  /* Allocate the global-ctx object */
  ncpus = z_align_up(z_system_processors(), 2);
  mmsize = sizeof(z_global_context_t) + ((ncpus - 1) * sizeof(struct cpu_ctx));
  __global_ctx = z_allocator_alloc(allocator, z_global_context_t, mmsize);
  if (__global_ctx == NULL) {
    Z_LOG_FATAL("unable to allocate the global context.");
    return(1);
  }

  /* Initialize the global context */
  __global_ctx->allocator = allocator;
  __global_ctx->user_data = user_data;

  __global_ctx->req_id = 0;
  for (i = 0; i < __NUM_WORK_QUEUES; ++i) {
    z_task_tree_open(&__global_ctx->pending[i]);
    z_task_queue_open(&__global_ctx->queue[i]);
  }

  if (z_thread_pool_open(&(__global_ctx->thread_pool), ncpus)) {
    Z_LOG_FATAL("unable to initialize the global context thread-pool.");
    z_allocator_free(allocator, __global_ctx);
    __global_ctx = NULL;
    return(2);
  }

  /* Initialize cpu context */
  cpu_ctx = __global_ctx->cpus;
  while (ncpus--) {
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
  z_thread_pool_wait(&(__global_ctx->thread_pool));
  return(0);
}

void z_global_context_close (void) {
  int i;

  /* clear queue */
  z_thread_pool_stop(&(__global_ctx->thread_pool));
  while (__global_ctx->thread_pool.total_threads--) {
    __cpu_ctx_close(&(__global_ctx->cpus[__global_ctx->thread_pool.total_threads]));
  }
  z_thread_pool_close(&(__global_ctx->thread_pool));

  for (i = 0; i < __NUM_WORK_QUEUES; ++i) {
    z_task_tree_close(&__global_ctx->pending[i]);
    z_task_queue_close(&__global_ctx->queue[i]);
  }

  z_allocator_free(__global_ctx->allocator, __global_ctx);
  __global_ctx = NULL;

  /* Uninitialize Debug env */
  z_debug_close();
}

void z_global_context_stop (void) {
  z_thread_pool_stop(&(__global_ctx->thread_pool));
}

void *z_global_context_user_data (void) {
  return(__global_ctx->user_data);
}

z_memory_t *z_global_memory (void) {
  return(&(__current_cpu_ctx()->memory));
}

void z_global_add_task (z_task_t *task) {
  if (task != NULL) {
    const int slot = __current_queue_slot();
    z_mutex_lock(&(__global_ctx->thread_pool.mutex));
    task->seqid = __global_ctx->req_id++;
    z_task_queue_push(&(__global_ctx->queue[slot]), task);
    z_wait_cond_signal(&(__global_ctx->thread_pool.task_ready));
    z_mutex_unlock(&(__global_ctx->thread_pool.mutex));
  }
}

void z_global_add_pending_tasks (z_task_t *tasks) {
  return(z_global_add_pending_ntasks(1, tasks));
}

void z_global_add_pending_ntasks (int count, ...) {
  const int slot = __current_queue_slot();
  int ntasks = 0;
  va_list ap;

  do {
    int i;
    va_start(ap, count);
    for (i = 0; i < count; ++i) {
      z_task_t *tasks = va_arg(ap, z_task_t *);
      ntasks += (tasks != NULL);
    }
    va_end(ap);
  } while (0);

  if (ntasks > 0) {
    va_start(ap, count);
    z_mutex_lock(&(__global_ctx->thread_pool.mutex));
    while (count--) {
      z_task_t *tasks = va_arg(ap, z_task_t *);
      z_task_tree_push(&(__global_ctx->pending[slot]), tasks);
    }
    if (ntasks > 1) {
      z_wait_cond_broadcast(&(__global_ctx->thread_pool.task_ready));
    } else {
      z_wait_cond_signal(&(__global_ctx->thread_pool.task_ready));
    }
    z_mutex_unlock(&(__global_ctx->thread_pool.mutex));
    va_end(ap);
  }
}
