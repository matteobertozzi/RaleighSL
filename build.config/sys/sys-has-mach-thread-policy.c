#include <stdlib.h>

#include <mach/thread_act.h>
#include <mach/thread_policy.h>
#include <pthread.h>

static void *__thread_func (void *arg) {
  return(arg);
}

int main (int argc, char **argv) {
  struct thread_affinity_policy policy;
  pthread_t thread;

  pthread_create(&thread, NULL, __thread_func, NULL);

  policy.affinity_tag = 0;
  thread_policy_set(pthread_mach_thread_np(thread), THREAD_AFFINITY_POLICY,
                    (thread_policy_t)&policy, THREAD_AFFINITY_POLICY_COUNT);

  pthread_join(thread, NULL);

  return(0);
}

