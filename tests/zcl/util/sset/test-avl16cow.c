#include <zcl/murmur3.h>
#include <zcl/keycmp.h>
#include <zcl/avl.h>

#include "test-sset.h"

#include <string.h>

#define BLOCK       (1024 << 10)
#define CMPFUNC     z_keycmp_mm32

static void test_perf (uint32_t isize) {
  z_test_sset_entry_t entry;
  uint32_t max_items;
  uint8_t *block;

  block = (uint8_t *)malloc(BLOCK);
  if (block == NULL) {
    perror("malloc()");
    return;
  }

  max_items = z_avl16cow_init(block, BLOCK, isize);
  max_items -= 20;
  Z_DEBUG("============================ PERF block=%u items=%u isize=%u",
          BLOCK, max_items, isize);

  uint32_t seqid = 0;
  z_test_sset_perf(max_items, max_items * 2, &entry,
  /* insert */ {
    z_avl16_txn_t txn;
    void *ptr;
    z_avl16_txn_open(&txn, block, 0);
    ptr = z_avl16_txn_insert(&txn, block, CMPFUNC, &(entry.key), NULL);
    Z_ASSERT(ptr != NULL, "unable to insert item %u", seqid);
    memcpy(ptr, &entry, isize);
    z_avl16_txn_commit(&txn, block);

    z_avl16cow_clean_all(block);
    seqid++;
  },
  /* append */ {
    z_avl16_txn_t txn;
    void *ptr;
    z_avl16_txn_open(&txn, block, 0);
    ptr = z_avl16_txn_append(&txn, block);
    Z_ASSERT(ptr != NULL, "unable to append item");
    memcpy(ptr, &entry, isize);
    z_avl16_txn_commit(&txn, block);
    z_avl16cow_clean_all(block);
  },
  /* lookup */ {
    z_avl16_txn_t txn;
    z_avl16_txn_open(&txn, block, 0);
    z_avl16_txn_lookup(&txn, block, CMPFUNC, &(entry.key), NULL);
  },
  /* remove */ {
    z_avl16_txn_t txn;
    z_avl16_txn_open(&txn, block, 0);
    z_avl16_txn_remove(&txn, block, CMPFUNC, &(entry.key), NULL);
    z_avl16_txn_commit(&txn, block);
    z_avl16cow_clean_all(block);
  },
  /* reset */ {
    z_avl16cow_init(block, BLOCK, isize);
  });

  free(block);
}

static void test_validate_v1 (uint32_t isize) {
  z_test_sset_entry_t lkpentry;
  z_test_sset_entry_t entry;
  uint8_t block[32 << 10];
  uint32_t max_items;

  max_items = z_avl16cow_init(block, sizeof(block), isize);
  max_items -= 20;
  Z_DEBUG("============================ VALIDATE-1 block=%u items=%u isize=%u",
          sizeof(block), max_items, isize);

  z_test_sset_validate(max_items, &entry, &lkpentry,
  /* insert */ {
    z_avl16_txn_t txn;
    void *ptr;
    z_avl16_txn_open(&txn, block, 0);
    ptr = z_avl16_txn_insert(&txn, block, CMPFUNC, &(entry.key), NULL);
    Z_ASSERT(ptr != NULL, "unable to insert item");
    memcpy(ptr, &entry, isize);
    z_avl16_txn_commit(&txn, block);
    z_avl16cow_clean_all(block);
  },
  /* append */ {
    z_avl16_txn_t txn;
    void *ptr;
    z_avl16_txn_open(&txn, block, 0);
    ptr = z_avl16_txn_append(&txn, block);
    Z_ASSERT(ptr != NULL, "unable to append item");
    memcpy(ptr, &entry, isize);
    z_avl16_txn_commit(&txn, block);
    z_avl16cow_clean_all(block);
  },
  /* lookup */ {
    z_test_sset_entry_t *res;
    z_avl16_txn_t txn;
    z_avl16_txn_open(&txn, block, 0);
    res = z_avl16_txn_lookup(&txn, block, CMPFUNC, &(lkpentry.key), NULL);
    z_test_sset_validate_entry("lookup", isize, res, &lkpentry);
  },
  /* lookup-miss */ {
    z_test_sset_entry_t *res;
    z_avl16_txn_t txn;
    z_avl16_txn_open(&txn, block, 0);
    res = z_avl16_txn_lookup(&txn, block, CMPFUNC, &(lkpentry.key), NULL);
    z_test_sset_validate_emiss("lkpmis", isize, res, &lkpentry);
  },
  /* remove */ {
    z_test_sset_entry_t *res;
    z_avl16_txn_t txn;
    z_avl16_txn_open(&txn, block, 0);
    res = z_avl16_txn_remove(&txn, block, CMPFUNC, &(entry.key), NULL);
    z_test_sset_validate_entry("remove", isize, res, &entry);
    z_avl16_txn_commit(&txn, block);
    z_avl16cow_clean_all(block);
  },
  /* reset */ {
    z_avl16cow_init(block, sizeof(block), isize);
  });
}

/* TODO */
static void test_validate_vN (uint32_t isize) {
  uint8_t block[512 << 10];
  uint32_t max_items;
  uint32_t i, j, k;

  z_avl16cow_init(block, sizeof(block), isize);
  max_items = 200;
  Z_DEBUG("============================ VALIDATE-N block=%u items=%u isize=%u",
          sizeof(block), max_items, isize);

  /* insert */
  for (i = 0; i < max_items; ++i) {
    z_test_sset_entry_t entry;
    z_test_sset_entry_t *res;
    z_avl16_txn_t txn;

    entry.key = z_bswap32(i);
    entry.value = i * 10;
    entry.step = i;

    z_avl16_txn_open(&txn, block, 0);
    res = z_avl16_txn_insert(&txn, block, CMPFUNC, &(entry.key), NULL);
    Z_ASSERT(res != NULL, "unable to insert item");
    memcpy(res, &entry, isize);
    z_avl16_txn_commit(&txn, block);

    for (j = 0; j <= i; ++j) {
      z_avl16_txn_open(&txn, block, j + 1);
      for (k = 0; k < j; ++k) {
        entry.key = z_bswap32(k);
        entry.value = k * 10;
        entry.step = k;

        res = z_avl16_txn_lookup(&txn, block, CMPFUNC, &(entry.key), NULL);
        z_test_sset_validate_entry("lookup", isize, res, &entry);
      }

      entry.key = z_bswap32(j);
      entry.value = j * 10;
      entry.step = j;
      res = z_avl16_txn_lookup(&txn, block, CMPFUNC, &(entry.key), NULL);
      z_test_sset_validate_emiss("lkpmis", isize, res, &entry);
    }
  }

  /* clean */
  for (i = 1; i <= max_items; ++i) {
    z_avl16_txn_t txn;
    z_avl16cow_clean(block, i);

    Z_ASSERT(z_avl16_txn_open(&txn, block, i),
             "seq=%"PRIu64" was removed", i);

    for (j = i+1; j <= max_items; ++j) {
      z_test_sset_entry_t entry;
      z_test_sset_entry_t *res;
      int err;

      err = z_avl16_txn_open(&txn, block, j);
      Z_ASSERT(!err, "unable to open seq=%"PRIu64, j);
      for (k = 0; k < j-1; ++k) {
        entry.key = z_bswap32(k);
        entry.value = k * 10;
        entry.step = k;

        res = z_avl16_txn_lookup(&txn, block, CMPFUNC, &(entry.key), NULL);
        z_test_sset_validate_entry("lookup", isize, res, &entry);
      }

      entry.key = z_bswap32(j);
      entry.value = j * 10;
      entry.step = j;
      res = z_avl16_txn_lookup(&txn, block, CMPFUNC, &(entry.key), NULL);
      z_test_sset_validate_emiss("lkpmis", isize, res, &entry);
    }
  }

  /* remove */
  for (i = 0; i < max_items; ++i) {
    z_test_sset_entry_t entry;
    z_test_sset_entry_t *res;
    z_avl16_txn_t txn;
    uint64_t seqid;

    entry.key = z_bswap32(i);
    entry.value = i * 10;
    entry.step = i;

    z_avl16_txn_open(&txn, block, 0);
    res = z_avl16_txn_remove(&txn, block, CMPFUNC, &(entry.key), NULL);
    Z_ASSERT(res != NULL, "unable to insert item");
    z_test_sset_validate_entry("lookup", isize, res, &entry);
    z_avl16_txn_commit(&txn, block);

    seqid = txn.seqid - i;
    for (j = 0; j <= i; ++j) {
      int err;

      err = z_avl16_txn_open(&txn, block, seqid + j);
      Z_ASSERT(!err, "unable to open seq=%"PRIu64, seqid + j);
      for (k = max_items - 1; k > j; --k) {
        entry.key = z_bswap32(k);
        entry.value = k * 10;
        entry.step = k;

        res = z_avl16_txn_lookup(&txn, block, CMPFUNC, &(entry.key), NULL);
        z_test_sset_validate_entry("lookup", isize, res, &entry);
      }

      entry.key = z_bswap32(j);
      entry.value = j * 10;
      entry.step = j;
      res = z_avl16_txn_lookup(&txn, block, CMPFUNC, &(entry.key), NULL);
      z_test_sset_validate_emiss("lkpmis", isize, res, &entry);
    }
  }
}

int main (int argc, char **argv) {
  test_validate_v1(4);
  test_validate_v1(8);

  test_validate_vN(4);
  test_validate_vN(8);

  test_perf(4);
  test_perf(8);

  return(0);
}
