#include <zcl/hashmap.h>
#include <zcl/murmur3.h>
#include <zcl/keycmp.h>

#include "test-map.h"

#include <string.h>

#define FANOUT              (16)
#define BUCKET_PAGE_SIZE    (64 << 10)
#define ENTRIES_PAGE_SIZE   (128 << 10)
#define INITIAL_CAPACITY    (1 << 8)
#define NITEMS_PERF         (1 << 20)
#define NITEMS_VALIDATION   (1 << 10)
#define HASHFUNC    z_hash32_murmur3
#define CMPFUNC     z_keycmp_mm32

static void test_perf (uint32_t isize) {
  uint32_t max_items = NITEMS_PERF;
  z_test_map_entry_t entry;
  z_hash_map_t hashmap;

  z_hash_map_open(&hashmap, isize, INITIAL_CAPACITY,
                  BUCKET_PAGE_SIZE, ENTRIES_PAGE_SIZE);
  Z_DEBUG("============================ PERF items=%u isize=%u", max_items, isize);

  z_test_map_perf(max_items, max_items * 2, &entry, HASHFUNC,
  /* insert */ {
    uint8_t *pkey = z_hash_map_put(&hashmap, entry.hash, CMPFUNC, &(entry.key), NULL);
    memcpy(pkey, &entry, isize);
  },
  /* lookup */ {
    z_hash_map_get(&hashmap, entry.hash, CMPFUNC, &(entry.key), NULL);
  },
  /* remove */ {
    z_hash_map_remove(&hashmap, entry.hash, CMPFUNC, &(entry.key), NULL);
  },
  /* reset */ {
    z_hash_map_open(&hashmap, isize, INITIAL_CAPACITY,
                  BUCKET_PAGE_SIZE, ENTRIES_PAGE_SIZE);
  });
}

static void test_validate (uint32_t isize) {
  uint32_t max_items = NITEMS_VALIDATION;
  z_test_map_entry_t lkpentry;
  z_test_map_entry_t entry;
  z_hash_map_t hashmap;

  z_hash_map_open(&hashmap, isize, INITIAL_CAPACITY,
                  BUCKET_PAGE_SIZE, ENTRIES_PAGE_SIZE);
  Z_DEBUG("============================ VALIDATE items=%u isize=%u", max_items, isize);

  z_test_map_validate(max_items, &entry, &lkpentry, HASHFUNC,
  /* insert */ {
    z_test_map_entry_t *pkey = z_hash_map_put(&hashmap, entry.hash, CMPFUNC, &(entry.key), NULL);
    memcpy(pkey, &entry, isize);
  },
  /* lookup */ {
    z_test_map_entry_t *res = z_hash_map_get(&hashmap, lkpentry.hash, CMPFUNC, &(lkpentry.key), NULL);
    z_test_map_validate_entry("lookup", isize, res, &lkpentry);
  },
  /* lookup-miss */ {
    z_test_map_entry_t *res = z_hash_map_get(&hashmap, lkpentry.hash, CMPFUNC, &(lkpentry.key), NULL);
    z_test_map_validate_emiss("lkpmis", isize, res, &lkpentry);
  },
  /* remove */ {
    z_test_map_entry_t *res = z_hash_map_remove(&hashmap, entry.hash, CMPFUNC, &(entry.key), NULL);
    z_test_map_validate_entry("remove", isize, res, &entry);
  },
  /* reset */ {
    z_hash_map_open(&hashmap, isize, INITIAL_CAPACITY,
                    BUCKET_PAGE_SIZE, ENTRIES_PAGE_SIZE);
  });
}

int main (int argc, char **argv) {
  test_validate(4);
  test_validate(8);
  test_validate(12);

  test_perf(4);
  test_perf(8);
  test_perf(12);
  return(0);
}
