#include <zcl/task-rq.h>
#include <zcl/atomic.h>
#include <zcl/global.h>
#include <zcl/thread.h>
#include <zcl/task.h>
#include <zcl/time.h>
#include <unistd.h>
#include <string.h>

#define __NEVENTS        (10ll)
#define __NUSERS         (2)
#define __NLOOPS         (10)

static void __dumpq (z_task_rq_rr_t *rq) {
  z_ticket_acquire(&(rq->rq.lock));
  z_vtask_queue_dump(&(rq->queue), stderr);
  z_ticket_release(&(rq->rq.lock));
}

struct noop_task {
  z_task_t vtask;
  z_task_rq_group_t *group;
};

static z_task_rstate_t __noop_task (z_task_t *vtask) {
  struct noop_task *task = z_container_of(vtask, struct noop_task, vtask);
  fprintf(stderr, "NOOP %p %p\n", vtask, task->group);
  z_task_group_close(task->group);
  memset(task->group, 0xff, sizeof(z_task_rq_group_t));
  return(Z_TASK_COMPLETED);
}

static void *__events_thread (void *args) {
  z_task_rq_rr_t user_rr[__NUSERS];
  long i, j, k;
  for (i = 0; i < __NLOOPS; ++i) {
    fprintf(stderr, "================================================ %ld\n", i);
    __dumpq(Z_CAST(z_task_rq_rr_t, z_global_rq()));
    for (k = 0; k < __NUSERS; ++k) {
      fprintf(stderr, "OPEN USER-RR[%ld] %p QUEUE %p\n", k, &(user_rr[k].rq.vtask), &(user_rr[k].queue));
      z_task_rq_open(&(user_rr[k].rq), &z_task_rq_rr, 0);
      __dumpq(&(user_rr[k]));
      z_task_rq_add(z_global_rq(), &(user_rr[k].rq.vtask));
      __dumpq(&(user_rr[k]));
      __dumpq(Z_CAST(z_task_rq_rr_t, z_global_rq()));
    }
    usleep(1000);
#if 1
    z_task_rq_group_t groups[__NEVENTS * __NUSERS];
    struct noop_task tasks[__NEVENTS * __NUSERS];
    long z = 0;
    for (j = 0; j < __NEVENTS; ++j) {
      for (k = 0; k < __NUSERS; ++k) {
        struct noop_task *task = &(tasks[z]);
        task->group = &(groups[z]);
        z_task_reset(&(task->vtask), __noop_task);
        task->vtask.vtask.flags.autoclean = 1;
        z_task_group_open(task->group, 0);
        z_task_rq_add(&(user_rr[k].rq), &(task->group->rq.vtask));
        z_task_group_add_barrier(task->group, &(task->vtask.vtask));
        z++;
      }
    }
    usleep(1000);
#endif
    for (k = 0; k < __NUSERS; ++k) {
      fprintf(stderr, "CLOSE USER-RR[%ld] %p\n", k, &(user_rr[k].rq.vtask));
      __dumpq(Z_CAST(z_task_rq_rr_t, z_global_rq()));
      z_task_rq_close(&(user_rr[k].rq));
      __dumpq(&(user_rr[k]));
      memset(&(user_rr[k].rq), 0xff, sizeof(z_task_rq_rr_t));
      __dumpq(Z_CAST(z_task_rq_rr_t, z_global_rq()));
    }
  }
  fprintf(stderr, "DONE\n");
  return(NULL);
}

int main (int argc, char **argv) {
  z_allocator_t allocator;

  /* Initialize allocator */
  if (z_system_allocator_open(&allocator))
    return(1);

  /* Initialize global context */
  if (z_global_context_open(&allocator, NULL)) {
    z_allocator_close(&allocator);
    return(1);
  }

  if (argc > 1) {
    z_thread_t tid;
    z_thread_start(&tid, __events_thread, NULL);
    z_thread_join(&tid);
  }

  z_global_context_stop();
  z_global_context_close();
  z_allocator_close(&allocator);
  return(0);
}
