#include <zcl/chainmap.h>
#include <zcl/murmur3.h>
#include <zcl/keycmp.h>

#include "test-map.h"

#include <string.h>

#define BLOCK       (656 << 10)
#define HASHFUNC    z_hash32_murmur3
#define CMPFUNC     z_keycmp_mm32

static void test_perf (uint32_t isize) {
  z_test_map_entry_t entry;
  uint32_t max_items;
  uint8_t *block;

  block = (uint8_t *)malloc(BLOCK);

  max_items = z_chain16_map_init(block, BLOCK, isize, 8);
  Z_DEBUG("============================ PERF block=%u items=%u isize=%u", block, max_items, isize);

  z_test_map_perf(max_items, max_items * 2, &entry, HASHFUNC,
  /* insert */ {
    uint8_t *pkey = z_chain16_map_put(block, entry.hash, CMPFUNC, &(entry.key), NULL);
    memcpy(pkey, &entry, isize);
  },
  /* lookup */ {
    z_chain16_map_get(block, entry.hash, CMPFUNC, &(entry.key), NULL);
  },
  /* remove */ {
    z_chain16_map_remove(block, entry.hash, CMPFUNC, &(entry.key), NULL);
  },
  /* reset */ {
    z_chain16_map_init(block, BLOCK, isize, 8);
  });

  free(block);
}

static void test_validate (uint32_t isize) {
  z_test_map_entry_t lkpentry;
  z_test_map_entry_t entry;
  uint8_t block[16 << 10];
  uint32_t max_items;

  max_items = z_chain16_map_init(block, sizeof(block), isize, 8);
  Z_DEBUG("============================ VALIDATE block=%u items=%u isize=%u", block, max_items, isize);

  z_test_map_validate(max_items, &entry, &lkpentry, HASHFUNC,
  /* insert */ {
    uint8_t *pkey = z_chain16_map_put(block, entry.hash, CMPFUNC, &(entry.key), NULL);
    memcpy(pkey, &entry, isize);
  },
  /* lookup */ {
    z_test_map_entry_t *res = z_chain16_map_get(block, lkpentry.hash, CMPFUNC, &(lkpentry.key), NULL);
    z_test_map_validate_entry("lookup", isize, res, &lkpentry);
  },
  /* lookup-miss */ {
    z_test_map_entry_t *res = z_chain16_map_get(block, lkpentry.hash, CMPFUNC, &(lkpentry.key), NULL);
    z_test_map_validate_emiss("lkpmis", isize, res, &lkpentry);
  },
  /* remove */ {
    z_test_map_entry_t *res = z_chain16_map_remove(block, entry.hash, CMPFUNC, &(entry.key), NULL);
    z_test_map_validate_entry("remove", isize, res, &entry);
  },
  /* reset */ {
    z_chain16_map_init(block, sizeof(block), isize, 8);
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
