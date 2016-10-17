#include <stdlib.h>
#include <pthread.h>

static void *__thread_func (void *arg) {
  return(arg);
}

int main (int argc, char **argv) {
  pthread_t thread;
  cpu_set_t cpuset;
  int core = 0;

  CPU_ZERO(&cpuset);
  CPU_SET(core, &cpuset);

  pthread_create(&thread, NULL, __thread_func, NULL);
  pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
  pthread_join(thread, NULL);

  return(0);
}

