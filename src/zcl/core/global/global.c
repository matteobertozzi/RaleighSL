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
#include <zcl/atomic.h>
#include <zcl/debug.h>

#define __NUM_WORK_QUEUES         (1)

struct cpu_ctx {
  z_memory_t memory;

  z_thread_t thread;
};

struct z_global_context {
  z_mutex_t mutex;
  z_wait_cond_t task_ready;
  z_wait_cond_t no_active_threads;

  z_task_tree_t  pending[__NUM_WORK_QUEUES];
  z_task_queue_t queue[__NUM_WORK_QUEUES];

  int is_closed;
  unsigned int waiting_threads;
  unsigned int ncpus;

  z_allocator_t *allocator;
  void *         user_data;

  struct cpu_ctx cpus[1];
};

static z_global_context_t *__global_ctx = NULL;

static z_task_t *__cpu_ctx_fetch_task (struct cpu_ctx *cpu_ctx) {
  z_task_t *task;
  int q;

  q = __NUM_WORK_QUEUES;
  while (q-- > 0) {
    if ((task = z_task_tree_pop(&(__global_ctx->pending[q]))) != NULL)
      return(task);
  }

  q = __NUM_WORK_QUEUES;
  while (q-- > 0) {
    if ((task = z_task_queue_pop(&(__global_ctx->queue[q]))) != NULL)
      return(task);
  }

  return(NULL);
}

static z_task_t *__cpu_ctx_wait_task (struct cpu_ctx *cpu_ctx) {
  z_task_t *task;

  z_mutex_lock(&(__global_ctx->mutex));

  if (__global_ctx->is_closed) {
    z_mutex_unlock(&(__global_ctx->mutex));
    return(0);
  }

  if ((task = __cpu_ctx_fetch_task(cpu_ctx)) == NULL) {
    if (++__global_ctx->waiting_threads == __global_ctx->ncpus) {
      z_wait_cond_broadcast(&(__global_ctx->no_active_threads));
    }
    z_wait_cond_wait(&(__global_ctx->task_ready), &(__global_ctx->mutex), 0);
  }

  z_mutex_unlock(&(__global_ctx->mutex));
  return(task);
}

static void *__cpu_ctx_loop (void *cpu_ctx) {
  while (!__global_ctx->is_closed) {
    z_task_t *task;
    if ((task = __cpu_ctx_wait_task((struct cpu_ctx *)cpu_ctx)) != NULL) {
      z_task_exec(task);
    }
  }

  z_mutex_lock(&(__global_ctx->mutex));
  if (++__global_ctx->waiting_threads == __global_ctx->ncpus) {
    z_wait_cond_broadcast(&(__global_ctx->no_active_threads));
  }
  z_mutex_unlock(&(__global_ctx->mutex));
  return(NULL);
}

static int __cpu_ctx_open (struct cpu_ctx *cpu_ctx, int cpu_id) {
  /* Create the cpu context thread */
  if (z_thread_start(&(cpu_ctx->thread), __cpu_ctx_loop, cpu_ctx)) {
    Z_LOG_FATAL("unable to initialize the thread for cpu %u.", cpu_id);
    return(1);
  }

  /* Bind the cpu context to the specified core */
  z_thread_bind_to_core(&(cpu_ctx->thread), cpu_id);

  /* Initialize Memory */
  z_memory_open(&(cpu_ctx->memory), __global_ctx->allocator);

  return(0);
}

static void __cpu_ctx_close (struct cpu_ctx *cpu_ctx) {
  z_memory_close(&(cpu_ctx->memory));
  z_thread_join(&(cpu_ctx->thread));
}

int z_global_context_open (z_allocator_t *allocator, void *user_data) {
  struct cpu_ctx *cpu_ctx;
  unsigned int mmsize;
  unsigned int ncpus;
  int i;

  if (__global_ctx != NULL) {
    Z_LOG_WARN("global context exists already!");
    return(0);
  }

  ncpus = z_system_processors();
  mmsize = sizeof(z_global_context_t) + ((ncpus - 1) * sizeof(struct cpu_ctx));
  __global_ctx = z_allocator_alloc(allocator, z_global_context_t, mmsize);
  if (__global_ctx == NULL) {
    Z_LOG_FATAL("unable to allocate the global context.");
    return(1);
  }

  /* Initialize the global context */
  __global_ctx->allocator = allocator;
  __global_ctx->user_data = user_data;
  __global_ctx->is_closed = 0;
  __global_ctx->waiting_threads = 0;
  __global_ctx->ncpus = ncpus;

  for (i = 0; i < __NUM_WORK_QUEUES; ++i) {
    z_task_tree_open(&__global_ctx->pending[i]);
    z_task_queue_open(&__global_ctx->queue[i]);
  }

  if (z_mutex_alloc(&(__global_ctx->mutex))) {
    Z_LOG_FATAL("unable to initialize the global context mutex.");
    z_allocator_free(allocator, __global_ctx);
    __global_ctx = NULL;
    return(2);
  }

  /* Initialize the global task queue */
  if (z_wait_cond_alloc(&(__global_ctx->task_ready))) {
    Z_LOG_FATAL("unable to initialize the global context wait condition 0.");
    z_mutex_free(&(__global_ctx->mutex));
    z_allocator_free(allocator, __global_ctx);
    __global_ctx = NULL;
    return(3);
  }

  if (z_wait_cond_alloc(&(__global_ctx->no_active_threads))) {
    Z_LOG_FATAL("unable to initialize the global context wait condition 1.");
    z_wait_cond_free(&(__global_ctx->task_ready));
    z_mutex_free(&(__global_ctx->mutex));
    z_allocator_free(allocator, __global_ctx);
    __global_ctx = NULL;
    return(4);
  }

  /* Initialize cpu context */
  cpu_ctx = __global_ctx->cpus;
  while (ncpus--) {
    if (__cpu_ctx_open(cpu_ctx++, ncpus)) {
      while (++ncpus < __global_ctx->ncpus)
        __cpu_ctx_close(--cpu_ctx);

      z_wait_cond_free(&(__global_ctx->no_active_threads));
      z_wait_cond_free(&(__global_ctx->task_ready));
      z_mutex_free(&(__global_ctx->mutex));
      z_allocator_free(allocator, __global_ctx);
      __global_ctx = NULL;
      return(5);
    }
  }

  return(0);
}

void z_global_context_close (void) {
  int i;

  z_mutex_lock(&(__global_ctx->mutex));
  __global_ctx->is_closed = 1;

  /* clear queue */
  z_wait_cond_broadcast(&(__global_ctx->task_ready));
  if (__global_ctx->waiting_threads < __global_ctx->ncpus) {
    z_wait_cond_wait(&(__global_ctx->no_active_threads), &(__global_ctx->mutex), 0);
  }

  z_mutex_unlock(&(__global_ctx->mutex));

  while (__global_ctx->ncpus--) {
    __cpu_ctx_close(&(__global_ctx->cpus[__global_ctx->ncpus]));
  }

  for (i = 0; i < __NUM_WORK_QUEUES; ++i) {
    z_task_tree_close(&__global_ctx->pending[i]);
    z_task_queue_close(&__global_ctx->queue[i]);
  }

  z_allocator_free(__global_ctx->allocator, __global_ctx);
  __global_ctx = NULL;
}

void z_global_context_stop (void) {
  z_lock(&(__global_ctx->mutex), z_mutex, {
    __global_ctx->is_closed = 1;
  });
}

void *z_global_context_user_data (void) {
  return(__global_ctx->user_data);
}

z_memory_t *z_global_memory (void) {
  return(&(__global_ctx->cpus[z_thread_core()].memory));
}

void z_global_add_task (z_task_t *task) {
  int slot = z_thread_core() & (__NUM_WORK_QUEUES - 1);
  z_mutex_lock(&(__global_ctx->mutex));
  z_task_queue_push(&(__global_ctx->queue[slot]), task);
  z_wait_cond_signal(&(__global_ctx->task_ready));
  z_mutex_unlock(&(__global_ctx->mutex));
}

void z_global_add_pending_tasks (z_task_t *tasks) {
  return(z_global_add_pending_ntasks(1, tasks));
}

void z_global_add_pending_ntasks (int count, ...) {
  int slot = z_thread_core() & (__NUM_WORK_QUEUES - 1);
  va_list ap;
  va_start(ap, count);
  z_mutex_lock(&(__global_ctx->mutex));
  while (count--) {
    z_task_t *tasks = va_arg(ap, z_task_t *);
    z_task_tree_push(&(__global_ctx->pending[slot]), tasks);
  }
  z_wait_cond_signal(&(__global_ctx->task_ready));
  z_mutex_unlock(&(__global_ctx->mutex));
  va_end(ap);
}
