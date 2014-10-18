#include <zcl/system.h>
#include <zcl/atomic.h>
#include <zcl/humans.h>
#include <zcl/time.h>
#include <zcl/spmc.h>

#include <pthread.h>
#include <string.h>
#include <stdio.h>

#define MAX_RUNS      (200ull << 20)
#define RING_SIZE     64
#define NTHREADS      (4 - 1)

struct consumer {
  uint64_t index;
  uint64_t sum;
  uint8_t *spmc;
  uint64_t *seqid;
  pthread_t tid;
};

static void *__consumer (void *args) {
  struct consumer *consumer = (struct consumer *)args;
  uint8_t *spmc = consumer->spmc;
  uint64_t *sequencer = consumer->seqid;

  uint64_t seqid = z_atomic_load(sequencer);
  while (seqid < MAX_RUNS) {
    uint64_t cached_tail;

    while (seqid == (cached_tail = z_spmc_last_publishd(spmc))) {
      z_system_cpu_relax();
    }

    while (seqid < cached_tail) {
      seqid++;
      z_atomic_set(sequencer, seqid);
    }
  }
  return(NULL);
}
#include <zcl/debug.h>
int main (int argc, char **argv) {
  struct consumer consumers[NTHREADS];
  uint8_t *spmc;
  char buffer[64];
  uint64_t st;
  double sec;
  uint64_t i;

#if 0
  spmc = (struct spmc *) malloc(sizeof(struct spmc));
#else
  void *ptr;
  posix_memalign(&ptr, 64, 256 + NTHREADS * 128);
  spmc = (uint8_t *)ptr;
#endif
  z_spmc_init(spmc, 512 + NTHREADS * 256, RING_SIZE);
  printf("SPMC-ALIGN %"PRIu64" %"PRIu64"\n", (uint64_t)spmc, (uint64_t)spmc % 64);

  for (i = 0; i < NTHREADS; ++i) {
    consumers[i].index = i;
    consumers[i].spmc = spmc;
    consumers[i].seqid = z_spmc_add_consumer(spmc);
    pthread_create(&(consumers[i].tid), NULL, __consumer, consumers + i);
  }

  st = z_time_micros();
  for (i = 0; i <= MAX_RUNS; ++i) {
    uint64_t seqid;
#if 1
    while (z_spmc_try_next(spmc, &seqid))
      z_system_cpu_relax();
#else
    seqid = z_spmc_wait_next(spmc);
#endif
    z_spmc_publish(spmc, seqid);
#if 0
    printf("STUCK i=%"PRIu64" -> %"PRIu64"/%"PRIu64" (%"PRIu64"/%"PRIu64")\n",
           i, spmc->head, spmc->tail, spmc->head & RING_SIZE, spmc->tail & RING_SIZE);
#endif
  }
  sec = (z_time_micros() - st) / 1000000.0f;
  z_human_dops(buffer, sizeof(buffer), (1 + MAX_RUNS) / sec);
  Z_DEBUG("PUBLISH %.3fsec %s/sec", sec, buffer);

  for (i = 0; i < NTHREADS; ++i) {
    pthread_join(consumers[i].tid, NULL);
  }

  free(spmc);

  return(0);
}
