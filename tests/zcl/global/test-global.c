#include <zcl/atomic.h>
#include <zcl/global.h>
#include <zcl/system.h>
#include <zcl/task.h>
#include <zcl/time.h>
#include <unistd.h>

static int __counter = 0;

static z_task_rstate_t __test_func (z_task_t *task) {
  z_atomic_inc(&__counter);
#if 0
  int sum = 0;
  int i;

  for (i = 1; i <= 64; ++i) {
    sum += task->vtask.seqid % i;
  }

  if (sum % 123) {
    sum += 5;
  }
#endif
  //usleep(1);
  //usleep(50); // 128k/sec
  //fprintf(stderr, "EXEC TASK %p %ld\n", task, task->vtask.seqid);
  z_memory_struct_free(z_global_memory(), z_task_t, task);
  return(Z_TASK_COMPLETED);
}

static z_vtask_t *__new_task (int data) {
  z_task_t *task;
  task = z_memory_struct_alloc(z_global_memory(), z_task_t);
  z_task_reset(task, __test_func);
  return(&(task->vtask));
}

#define __NTASKS        (100 * 100000)
int main (int argc, char **argv) {
  z_allocator_t allocator;
  uint64_t stime, etime;

  /* Initialize allocator */
  if (z_system_allocator_open(&allocator))
    return(1);

  /* Initialize global context */
  if (z_global_context_open(&allocator, NULL)) {
    z_allocator_close(&allocator);
    return(1);
  }

  stime = z_time_millis();
  if (argc > 1) {
    int i;

    Z_LOG_TRACE("PUSHING TASKS!\n");
    for (i = 0; i < __NTASKS; ++i) {
      z_task_rq_add(z_global_rq(), __new_task(i));
    }

    while (__counter < __NTASKS) {
      usleep(1000);
      z_system_cpu_relax();
    }
  }

  etime = z_time_millis();

  z_global_context_stop();
  z_global_context_close();
  z_allocator_close(&allocator);
  fprintf(stderr, "EXECUTED %d (%.3freq/sec)\n", __counter, (float)__NTASKS / ((etime - stime) / 1000.0));
  return(0);
}
