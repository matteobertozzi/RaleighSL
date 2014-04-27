#include <zcl/atomic.h>
#include <zcl/global.h>
#include <zcl/task.h>
#include <zcl/time.h>
#include <unistd.h>

static long __counter = 0;

static void __test_func (z_task_t *task) {
  int sum = 0;
  int i;

  z_atomic_add(&__counter, task->vtask.seqid);

  for (i = 1; i <= 64; ++i) {
    sum += task->vtask.seqid % i;
  }

  if (sum % 123) {
    sum += 5;
  }

  //usleep(1);
  //usleep(50); // 128k/sec
  //fprintf(stderr, "EXEC %ld\n", task->data);
  z_memory_struct_free(z_global_memory(), z_task_t, task);
}

static z_vtask_t *__new_task (long data) {
  z_task_t *task;
  task = z_memory_struct_alloc(z_global_memory(), z_task_t);
  z_task_reset(task, __test_func);
  return(&(task->vtask));
}

#define __NTASKS        (50 * 100000)
int main (int argc, char **argv) {
  z_allocator_t allocator;
  uint64_t stime, etime;
  long expected = 0;

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
    for (long i = 1; i <= __NTASKS; ++i) {
      z_task_rq_add(z_global_rq(), __new_task(i));
      z_global_new_task_signal();
      expected += i;
    }

    while (__counter < expected)
      z_global_context_wait();
  }

  etime = z_time_millis();

  z_global_context_stop();
  z_global_context_close();
  z_allocator_close(&allocator);
  fprintf(stderr, "EXECUTED %ld (%.3freq/sec)\n", __counter, (float)__NTASKS / ((etime - stime) / 1000.0));
  fprintf(stderr, "EXPECTED %ld\n", expected);
  return(0);
}
