#include <zcl/murmur3.h>
#include <zcl/keycmp.h>
#include <zcl/avl16s.h>

#include "test-sset.h"

#include <string.h>

#define BLOCK       (576 << 10)
#define CMPFUNC     z_keycmp_mm32

static void test_perf (uint32_t isize) {
  z_test_sset_entry_t entry;
  uint32_t max_items;
  uint8_t *block;

  block = (uint8_t *)malloc(BLOCK);

  max_items = z_avl16s_init(block, BLOCK, isize);
  Z_DEBUG("============================ PERF block=%u items=%u isize=%u", block, max_items, isize);

  z_test_sset_perf(max_items, max_items * 2, &entry,
  /* insert */ {
    uint8_t *pkey = z_avl16s_insert(block, CMPFUNC, &(entry.key), NULL);
    memcpy(pkey, &entry, isize);
  },
  /* append */ {
    uint8_t *pkey = z_avl16s_append(block);
    memcpy(pkey, &entry, isize);
  },
  /* lookup */ {
    z_avl16s_lookup(block, CMPFUNC, &(entry.key), NULL);
  },
  /* remove */ {
    z_avl16s_remove(block, CMPFUNC, &(entry.key), NULL);
  },
  /* reset */ {
    z_avl16s_init(block, BLOCK, isize);
  });

  free(block);
}

static void test_perf_lookup (uint32_t isize) {
  z_test_sset_entry_t entry;
  z_avl16s_iter_t iter;
  uint32_t max_items;
  uint8_t *block;

  block = (uint8_t *)malloc(BLOCK);

  max_items = z_avl16s_init(block, BLOCK, isize);
  Z_DEBUG("============================ PERF block=%u items=%u isize=%u", block, max_items, isize);

  z_test_sset_iseq("add", 1, 0, max_items, &entry, {
    uint8_t *pkey = z_avl16s_insert(block, CMPFUNC, &(entry.key), NULL);
    memcpy(pkey, &entry, isize);
  });

  z_test_sset_iseq("get", 1, 0, max_items, &entry, {
    z_avl16s_lookup(block, CMPFUNC, &(entry.key), NULL);
  });

  z_avl16s_iter_init(&iter, block);
  z_test_sset_iseq("lseek", 1, 0, max_items, &entry, {
    z_avl16s_iter_seek_le(&iter, CMPFUNC, &(entry.key), NULL);
  });

  free(block);
}

static void test_validate (uint32_t isize) {
  z_test_sset_entry_t lkpentry;
  z_test_sset_entry_t entry;
  uint8_t block[16 << 10];
  uint32_t max_items;

  max_items = z_avl16s_init(block, sizeof(block), isize);
  Z_DEBUG("============================ VALIDATE block=%u items=%u isize=%u", block, max_items, isize);

  z_test_sset_validate(max_items, &entry, &lkpentry,
  /* insert */ {
    uint8_t *pkey = z_avl16s_insert(block, CMPFUNC, &(entry.key), NULL);
    memcpy(pkey, &entry, isize);
  },
  /* append */ {
    uint8_t *pkey = z_avl16s_append(block);
    memcpy(pkey, &entry, isize);
  },
  /* lookup */ {
    z_test_sset_entry_t *res = z_avl16s_lookup(block, CMPFUNC, &(lkpentry.key), NULL);
    z_test_sset_validate_entry("lookup", isize, res, &lkpentry);
  },
  /* lookup-miss */ {
    z_test_sset_entry_t *res = z_avl16s_lookup(block, CMPFUNC, &(lkpentry.key), NULL);
    z_test_sset_validate_emiss("lkpmis", isize, res, &lkpentry);
  },
  /* remove */ {
    z_test_sset_entry_t *res = z_avl16s_remove(block, CMPFUNC, &(entry.key), NULL);
    z_test_sset_validate_entry("remove", isize, res, &entry);
  },
  /* reset */ {
    z_avl16s_init(block, sizeof(block), isize);
  });
}

static void test_iter_validate (uint32_t isize) {
  z_test_sset_entry_t *pentry;
  z_test_sset_entry_t entry;
  uint8_t block[64 << 10];
  uint32_t i, max_items;
  z_avl16s_iter_t iter;

  max_items = z_avl16s_init(block, sizeof(block), isize);
  Z_DEBUG("============================ VALIDATE ITER block=%u items=%u isize=%u",
          block, max_items, isize);

  for (i = 0; i < max_items; i += 2) {
    uint8_t *pkey;
    entry.key = z_bswap32(i);
    entry.value = (entry.key | 0x49249249);
    entry.step = i;

    pkey = z_avl16s_insert(block, CMPFUNC, &(entry.key), NULL);
    memcpy(pkey, &entry, isize);
  }

  z_avl16s_iter_init(&iter, block);

  /* Validate next */
  pentry = z_avl16s_iter_seek_begin(&iter);
  for (i = 0; pentry != NULL; i += 2) {
    entry.key = z_bswap32(i);
    entry.value = (entry.key | 0x49249249);
    entry.step = i;

    z_test_sset_validate_entry("iter-next", isize, pentry, &entry);
    pentry = z_avl16s_iter_next(&iter);
  }

  /* Validate prev */
  pentry = z_avl16s_iter_seek_end(&iter);
  for (i -= 2; pentry != NULL; i -= 2) {
    entry.key = z_bswap32(i);
    entry.value = (entry.key | 0x49249249);
    entry.step = i;

    z_test_sset_validate_entry("iter-prev", isize, pentry, &entry);
    pentry = z_avl16s_iter_prev(&iter);
  }

  /* Validate near */
  for (i = 1; i < max_items - 1; i += 2) {
    entry.key = z_bswap32(i);
    entry.value = (entry.key | 0x49249249);
    entry.step = i;

    pentry = z_avl16s_iter_seek_le(&iter, CMPFUNC, &entry, NULL);
    if (pentry == NULL) {
      Z_DEBUG(" - iter-le %u VALUE NOT FOUND %u (i=%u)",
              z_bswap32(i), z_bswap32(i-1), i);
      abort();
    } else if (pentry->key != z_bswap32(i-1)) {
      Z_DEBUG(" - iter-le FOUND A BAD VALUE %u != %u",
              pentry->key, z_bswap32(i-1));
      abort();
    }

    pentry = z_avl16s_iter_seek_ge(&iter, CMPFUNC, &entry, NULL);
    if (pentry == NULL) {
      Z_DEBUG(" - iter-ge %u VALUE NOT FOUND %u (i=%u)",
              z_bswap32(i), z_bswap32(i+1), i);
      abort();
    } else if (pentry->key != z_bswap32(i+1)) {
      Z_DEBUG(" - iter-ge FOUND A BAD VALUE %u != %u",
              pentry->key, z_bswap32(i+1));
      abort();
    }
  }
}

int main (int argc, char **argv) {
  test_validate(4);
  test_validate(8);

  test_iter_validate(4);
  test_iter_validate(8);

  test_perf(4);
  test_perf(8);

  test_perf_lookup(4);
  return(0);
}
