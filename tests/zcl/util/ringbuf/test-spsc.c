#include <zcl/system.h>
#include <zcl/atomic.h>
#include <zcl/humans.h>
#include <zcl/time.h>
#include <zcl/spsc.h>

#include <pthread.h>
#include <string.h>
#include <stdio.h>

#define MAX_RUNS      (50ull << 20)
#define RING_SIZE     64
#define NTHREADS      (4 - 1)

struct consumer {
  uint64_t index;
  z_spsc_t *spsc;
  uint64_t  running;
  pthread_t tid;
  uint64_t _pad[4];
};

static void *__consumer (void *args) {
  struct consumer *consumer = (struct consumer *)args;
  z_spsc_t *spsc = consumer->spsc;

  while (consumer->running) {
    uint64_t seqid;

    z_spsc_try_acquire(spsc, &seqid);
  }
  return(NULL);
}
#include <zcl/debug.h>
int main (int argc, char **argv) {
  struct consumer consumers[NTHREADS];
  z_spsc_t spsc;
  char buffer[64];
  uint64_t st;
  double sec;
  uint64_t i;

  z_spsc_init(&spsc, RING_SIZE);

  for (i = 0; i < NTHREADS; ++i) {
    consumers[i].index = i;
    consumers[i].spsc = &spsc;
    consumers[i].running = 1;
    pthread_create(&(consumers[i].tid), NULL, __consumer, consumers + i);
  }

  st = z_time_micros();
  for (i = 0; i <= MAX_RUNS; ++i) {
    uint64_t seqid;
    while (z_spsc_try_next(&spsc, &seqid))
      z_system_cpu_relax();
    z_spsc_publish(&spsc, seqid);
#if 0
    printf("STUCK i=%"PRIu64" -> %"PRIu64"/%"PRIu64" (%"PRIu64"/%"PRIu64")\n",
           i, spmc->head, spmc->tail, spmc->head & RING_SIZE, spmc->tail & RING_SIZE);
#endif
  }
  sec = (z_time_micros() - st) / 1000000.0f;
  z_human_dops(buffer, sizeof(buffer), (1 + MAX_RUNS) / sec);
  Z_DEBUG("PUBLISH %.3fsec %s/sec", sec, buffer);

  while (z_atomic_load(&(spsc.gating_seq)) < z_atomic_load(&(spsc.cursor))) {
    z_system_cpu_relax();
  }

  for (i = 0; i < NTHREADS; ++i) {
    consumers[i].running = 0;
  }

  for (i = 0; i < NTHREADS; ++i) {
    pthread_join(consumers[i].tid, NULL);
  }
  return(0);
}
