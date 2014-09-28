#ifndef _Z_TEST_MAP_H_
#define _Z_TEST_MAP_H_

#include <zcl/endian.h>
#include <zcl/time.h>
#include <zcl/rand.h>
#include <zcl/math.h>

typedef struct z_test_map_entry z_test_map_entry_t;

struct z_test_map_entry {
  uint32_t key;
  uint32_t value;
  uint32_t hash;
  uint32_t step;
} __attribute__((packed));

#define z_test_map_rand(name, dump, start, end, pentry, hash_func, __code__)   \
  do {                                                                         \
    const uint64_t nitems = (end) - (start);                                   \
    uint64_t seed = (nitems);                                                  \
    uint32_t i;                                                                \
    for (i = 0; i < start; ++i) {                                              \
      z_rand32(&seed);                                                         \
    }                                                                          \
    z_timer_t timer;                                                           \
    z_timer_start(&timer);                                                     \
    while (i++ < end) {                                                        \
      (pentry)->key = z_rand32(&seed);                                         \
      (pentry)->key = z_bswap32((pentry)->key);                                \
      (pentry)->value = ((pentry)->key | 0x49249249);                          \
      (pentry)->hash = hash_func(seed, &((pentry)->key), 4);                   \
      (pentry)->step = i;                                                      \
      do __code__ while (0);                                                   \
    }                                                                          \
    z_timer_stop(&timer);                                                      \
    if (dump) {                                                                \
      Z_DEBUG("[RAND] %s executed in %.5fsec %8.3fM/sec "                      \
              "(nitems=%u micros=%"PRIu64")", name,                            \
              z_timer_secs(&timer), (nitems / z_timer_secs(&timer)) / 1000000, \
              nitems, z_timer_micros(&timer));                                 \
    }                                                                          \
  } while (0)


#define z_test_map_cycle(name, dump, start, end, nitems,                       \
                         pentry, hash_func, __code__)                          \
  do {                                                                         \
    const uint32_t prime = z_cycle32_prime(nitems);                            \
    uint32_t i, seed = (nitems);                                               \
    for (i = 0; i < (start); ++i) {                                            \
      z_cycle32(&seed, prime, nitems);                                         \
    }                                                                          \
    z_timer_t timer;                                                           \
    z_timer_start(&timer);                                                     \
    for (; i < end; ++i) {                                                     \
      (pentry)->key = z_cycle32(&seed, prime, nitems);                         \
      (pentry)->key = z_bswap32((pentry)->key);                                \
      (pentry)->value = ((pentry)->key | 0x49249249);                          \
      (pentry)->hash = hash_func(seed, &((pentry)->key), 4);                   \
      (pentry)->step = i;                                                      \
      do __code__ while (0);                                                   \
    }                                                                          \
    z_timer_stop(&timer);                                                      \
    if (dump) {                                                                \
      Z_DEBUG("[CYCL] %s executed in %.5fsec %8.3fM/sec "                      \
              "(nitems=%u micros=%"PRIu64")", name,                            \
              z_timer_secs(&timer), (nitems / z_timer_secs(&timer)) / 1000000, \
              nitems, z_timer_micros(&timer));                                 \
    }                                                                          \
  } while (0)

#define z_test_map_iseq(name, dump, start, end, pentry, hash_func, __code__)   \
  do {                                                                         \
    const uint64_t nitems = (end) - (start);                                   \
    uint32_t i;                                                                \
    z_timer_t timer;                                                           \
    z_timer_start(&timer);                                                     \
    for (i = start; i < end; ++i) {                                            \
      (pentry)->key = z_bswap32(i);                                            \
      (pentry)->value = ((pentry)->key | 0x49249249);                          \
      (pentry)->hash = hash_func(i, &((pentry)->key), 4);                      \
      (pentry)->step = i;                                                      \
      do __code__ while (0);                                                   \
    }                                                                          \
    z_timer_stop(&timer);                                                      \
    if (dump) {                                                                \
      Z_DEBUG("[ISEQ] %s executed in %.5fsec %8.3fM/sec "                      \
              "(nitems=%u micros=%"PRIu64")", name,                            \
              z_timer_secs(&timer), (nitems / z_timer_secs(&timer)) / 1000000, \
              nitems, z_timer_micros(&timer));                                 \
    }                                                                          \
  } while (0)

#define z_test_map_dseq(name, dump, start, end, nitems,                        \
                        pentry, hash_func, __code__)                           \
  do {                                                                         \
    uint32_t i;                                                                \
    z_timer_t timer;                                                           \
    z_timer_start(&timer);                                                     \
    for (i = start; i < end; ++i) {                                            \
      (pentry)->key = z_bswap32(nitems - i);                                   \
      (pentry)->value = ((pentry)->key | 0x49249249);                          \
      (pentry)->hash = hash_func(101, &((pentry)->key), 4);                    \
      (pentry)->step = i;                                                      \
      do __code__ while (0);                                                   \
    }                                                                          \
    z_timer_stop(&timer);                                                      \
    if (dump) {                                                                \
      Z_DEBUG("[DSEQ] %s executed in %.5fsec %8.3fM/sec "                      \
              "(nitems=%u micros=%"PRIu64")", name,                            \
              z_timer_secs(&timer), (nitems / z_timer_secs(&timer)) / 1000000, \
              nitems, z_timer_micros(&timer));                                 \
    }                                                                          \
  } while (0)

#define z_test_map_perf(count, xcount, pentry, hash_func, __icode__,                  \
                        __lcode__, __rcode__, __resetcode__)                          \
  do {                                                                                \
    z_test_map_rand("INSERT", 1, 0, count, pentry, hash_func, __icode__);             \
    z_test_map_rand("LOOKUP", 1, 0, count, pentry, hash_func, __lcode__);             \
    z_test_map_rand("REMOVE", 1, 0, count, pentry, hash_func, __rcode__);             \
                                                                                      \
    do __resetcode__ while(0);                                                        \
    z_test_map_iseq("INSERT", 1,     0,  count, pentry, hash_func, __icode__);        \
    z_test_map_iseq("LOOKUP", 1,     0,  count, pentry, hash_func, __lcode__);        \
    z_test_map_iseq("LKPMIS", 1, count, xcount, pentry, hash_func, __lcode__);        \
    z_test_map_iseq("REMOVE", 1,     0,  count, pentry, hash_func, __rcode__);        \
                                                                                      \
    do __resetcode__ while(0);                                                        \
    z_test_map_dseq("INSERT", 1,     0,  count, count, pentry, hash_func, __icode__); \
    z_test_map_dseq("LOOKUP", 1,     0,  count, count, pentry, hash_func, __lcode__); \
    z_test_map_dseq("LKPMIS", 1, count, xcount, count, pentry, hash_func, __lcode__); \
    z_test_map_dseq("REMOVE", 1,     0,  count, count, pentry, hash_func, __rcode__); \
  } while (0)

#define z_test_map_validate(count, pentry, pelkp, hash_func, __icode__,        \
                            __lcode__, __mcode__, __rcode__, __resetcode__)    \
  do {                                                                         \
    z_test_map_cycle("INSERT", 1, 0, count, count, pentry, hash_func, {        \
      do __icode__ while (0);                                                  \
      z_test_map_cycle("RLOOKUP", 0, 0, (pentry)->step + 1, count,             \
                       pelkp, hash_func, __lcode__);                           \
      z_test_map_cycle("RLKPMIS", 0, (pentry)->step + 1, count, count,         \
                       pelkp, hash_func, __mcode__);                           \
    });                                                                        \
    z_test_map_cycle("REMOVE", 1, 0, count, count, pentry, hash_func, {        \
      do __rcode__ while (0);                                                  \
      z_test_map_cycle("RLKPMIS", 0, 0, (pentry)->step, count,                 \
                       pelkp, hash_func, __mcode__);                           \
      z_test_map_cycle("RLOOKUP", 0, (pentry)->step + 1, count,  count,        \
                       pelkp, hash_func, __lcode__);                           \
    });                                                                        \
                                                                               \
    do __resetcode__ while(0);                                                 \
    z_test_map_iseq("INSERT", 1, 0, count, pentry, hash_func, {                \
      do __icode__ while (0);                                                  \
      z_test_map_iseq("RLOOKUP", 0, 0, (pentry)->step,                         \
                       pelkp, hash_func, __lcode__);                           \
      z_test_map_iseq("RLKPMIS", 0, (pentry)->step + 1, count,                 \
                       pelkp, hash_func, __mcode__);                           \
    });                                                                        \
    z_test_map_iseq("REMOVE", 1, 0, count, pentry, hash_func, {                \
      do __rcode__ while (0);                                                  \
      z_test_map_iseq("RLKPMIS", 0, 0, (pentry)->step,                         \
                       pelkp, hash_func, __mcode__);                           \
      z_test_map_iseq("RLOOKUP", 0, (pentry)->step + 1, count,                 \
                       pelkp, hash_func, __lcode__);                           \
    });                                                                        \
                                                                               \
    do __resetcode__ while(0);                                                 \
    z_test_map_dseq("INSERT", 1, 0, count, count, pentry, hash_func, {         \
      do __icode__ while (0);                                                  \
      z_test_map_dseq("RLOOKUP", 0, 0, (pentry)->step, count,                  \
                       pelkp, hash_func, __lcode__);                           \
      z_test_map_dseq("RLKPMIS", 0, (pentry)->step + 1, count, count,          \
                       pelkp, hash_func, __mcode__);                           \
    });                                                                        \
    z_test_map_dseq("REMOVE", 1, 0, count, count, pentry, hash_func, {         \
      do __rcode__ while (0);                                                  \
      z_test_map_dseq("RLKPMIS", 0, 0, (pentry)->step, count,                  \
                       pelkp, hash_func, __mcode__);                           \
      z_test_map_dseq("RLOOKUP", 0, (pentry)->step + 1, count, count,          \
                       pelkp, hash_func, __lcode__);                           \
    });                                                                        \
  } while (0)

#define z_test_map_validate_entry(name, isize, res, entry)                     \
  if (res == NULL) {                                                           \
    Z_DEBUG(" - %s NOT FOUND %u", name, (entry)->key);                         \
    abort();                                                                   \
  } else if ((entry)->key != (res)->key) {                                     \
    Z_DEBUG(" - %s FOUND A BAD KEY %u != %u",                                  \
            name, (entry)->key, (res)->key);                                   \
    abort();                                                                   \
  } else if (isize == 12 && (entry)->hash != (res)->hash) {                    \
    Z_DEBUG(" - %s FOUND A BAD HASH %u != %u",                                 \
            name, (entry)->hash, (res)->hash);                                 \
    abort();                                                                   \
  } else if (isize >= 8 && (entry)->value != res->value) {                     \
    Z_DEBUG(" - %s FOUND A BAD VALUE %u != %u",                                \
            name, (entry)->value, (res)->value);                               \
    abort();                                                                   \
  }

#define z_test_map_validate_emiss(name, isize, res, entry)                     \
  if (res != NULL) {                                                           \
    Z_DEBUG(" - %s FOUND %u for removed %u",                                   \
            name, (res)->key, (entry)->key);                                   \
    abort();                                                                   \
  }

#endif /* !_Z_TEST_MAP_H_ */
