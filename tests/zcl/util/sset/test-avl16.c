#include <zcl/murmur3.h>
#include <zcl/keycmp.h>
#include <zcl/avl.h>

#include "test-sset.h"

#include <string.h>

#define BLOCK       (576 << 10)
#define CMPFUNC     z_keycmp_mm32

static void test_perf (uint32_t isize) {
  z_test_sset_entry_t entry;
  uint32_t max_items;
  uint8_t *block;

  block = (uint8_t *)malloc(BLOCK);

  max_items = z_avl16_init(block, BLOCK, isize);
  Z_DEBUG("============================ PERF block=%u items=%u isize=%u", block, max_items, isize);

  z_test_sset_perf(max_items, max_items * 2, &entry,
  /* insert */ {
    uint8_t *pkey = z_avl16_insert(block, CMPFUNC, &(entry.key), NULL);
    memcpy(pkey, &entry, isize);
  },
  /* append */ {
    uint8_t *pkey = z_avl16_append(block);
    memcpy(pkey, &entry, isize);
  },
  /* lookup */ {
    z_avl16_lookup(block, CMPFUNC, &(entry.key), NULL);
  },
  /* remove */ {
    z_avl16_remove(block, CMPFUNC, &(entry.key), NULL);
  },
  /* reset */ {
    z_avl16_init(block, BLOCK, isize);
  });

  free(block);
}

static void test_validate (uint32_t isize) {
  z_test_sset_entry_t lkpentry;
  z_test_sset_entry_t entry;
  uint8_t block[16 << 10];
  uint32_t max_items;

  max_items = z_avl16_init(block, sizeof(block), isize);
  Z_DEBUG("============================ VALIDATE block=%u items=%u isize=%u", block, max_items, isize);

  z_test_sset_validate(max_items, &entry, &lkpentry,
  /* insert */ {
    uint8_t *pkey = z_avl16_insert(block, CMPFUNC, &(entry.key), NULL);
    memcpy(pkey, &entry, isize);
  },
  /* append */ {
    uint8_t *pkey = z_avl16_append(block);
    memcpy(pkey, &entry, isize);
  },
  /* lookup */ {
    z_test_sset_entry_t *res = z_avl16_lookup(block, CMPFUNC, &(lkpentry.key), NULL);
    z_test_sset_validate_entry("lookup", isize, res, &lkpentry);
  },
  /* lookup-miss */ {
    z_test_sset_entry_t *res = z_avl16_lookup(block, CMPFUNC, &(lkpentry.key), NULL);
    z_test_sset_validate_emiss("lkpmis", isize, res, &lkpentry);
  },
  /* remove */ {
    z_test_sset_entry_t *res = z_avl16_remove(block, CMPFUNC, &(entry.key), NULL);
    z_test_sset_validate_entry("remove", isize, res, &entry);
  },
  /* reset */ {
    z_avl16_init(block, sizeof(block), isize);
  });
}

int main (int argc, char **argv) {
  test_validate(4);
  test_validate(8);

  test_perf(4);
  test_perf(8);
  return(0);
}
