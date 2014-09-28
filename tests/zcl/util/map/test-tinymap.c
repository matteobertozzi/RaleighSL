#include <zcl/tinymap.h>
#include <zcl/murmur3.h>
#include <zcl/keycmp.h>

#include "test-map.h"

#include <string.h>

#define BLOCK       (512 << 10)
#define HASHFUNC    z_hash32_murmur3
#define CMPFUNC     z_keycmp_mm32

static void test_perf (uint32_t isize) {
  z_test_map_entry_t entry;
  uint8_t block[BLOCK];
  uint32_t max_items;

  max_items = z_tiny_map_init(block, BLOCK, isize);
  Z_DEBUG("============================ PERF block=%u items=%u isize=%u", block, max_items, isize);

  z_test_map_perf(max_items, max_items + 100, &entry, HASHFUNC,
  /* insert */ {
    uint8_t *pkey = z_tiny_map_put(block, entry.hash, CMPFUNC, &(entry.key), NULL);
    memcpy(pkey, &entry, isize);
  },
  /* lookup */ {
    z_tiny_map_get(block, entry.hash, CMPFUNC, &(entry.key), NULL);
  },
  /* remove */ {
    z_tiny_map_remove(block, entry.hash, CMPFUNC, &(entry.key), NULL);
  },
  /* reset */ {
    z_tiny_map_init(block, BLOCK, isize);
  });
}

static void test_validate (uint32_t isize) {
  z_test_map_entry_t lkpentry;
  z_test_map_entry_t entry;
  uint8_t block[4 << 10];
  uint32_t max_items;

  max_items = z_tiny_map_init(block, sizeof(block), isize);
  Z_DEBUG("============================ VALIDATE block=%u items=%u isize=%u", block, max_items, isize);

  z_test_map_validate(max_items, &entry, &lkpentry, HASHFUNC,
  /* insert */ {
    uint8_t *pkey = z_tiny_map_put(block, entry.hash, CMPFUNC, &(entry.key), NULL);
    memcpy(pkey, &entry, isize);
  },
  /* lookup */ {
    z_test_map_entry_t *res = z_tiny_map_get(block, lkpentry.hash, CMPFUNC, &(lkpentry.key), NULL);
    z_test_map_validate_entry("lookup", isize, res, &lkpentry);
  },
  /* lookup-miss */ {
    z_test_map_entry_t *res = z_tiny_map_get(block, lkpentry.hash, CMPFUNC, &(lkpentry.key), NULL);
    z_test_map_validate_emiss("lkpmis", isize, res, &lkpentry);
  },
  /* remove */ {
    z_test_map_entry_t *res = z_tiny_map_remove(block, entry.hash, CMPFUNC, &(entry.key), NULL);
    z_test_map_validate_entry("remove", isize, res, &entry);
  },
  /* reset */ {
    z_tiny_map_init(block, sizeof(block), isize);
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
