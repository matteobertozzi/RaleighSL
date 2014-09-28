#include <zcl/macros.h>
#include <zcl/endian.h>
#include <zcl/time.h>
#include <zcl/rand.h>
#include <zcl/sort.h>

#include <string.h>

#define NITEMS      20000

static void __verify_u32 (uint32_t array[]) {
  uint32_t i;
  for (i = 1; i < NITEMS; ++i) {
    if (array[i-1] > array[i]) {
      printf("BAD SORT %"PRIu32" %"PRIu32" -> %"PRIu32"\n", i, array[i-1], array[i]);
      break;
    }
  }
}

static void __verify_u64 (uint64_t array[]) {
  uint32_t i;
  for (i = 1; i < NITEMS; ++i) {
    if (array[i-1] > array[i]) {
      printf("BAD SORT %"PRIu32" %"PRIu64" -> %"PRIu64"\n", i, array[i-1], array[i]);
      break;
    }
  }
}

int main (int argc, char **argv) {
  uint64_t u64[NITEMS];
  uint32_t u32[NITEMS];
  uint64_t i, seed;

  /* quick3 random */
  seed = NITEMS; for (i = 0; i < NITEMS; ++i) u32[i] = z_bswap32(z_rand32(&seed));
  seed = NITEMS; for (i = 0; i < NITEMS; ++i) u64[i] = z_bswap64(z_rand64(&seed));

  Z_DEBUG_TOPS(u32_QUICK3_RND, NITEMS, {
    z_quick3_u32(u32, 0, NITEMS - 1);
  });
  __verify_u32(u32);

  Z_DEBUG_TOPS(u64_QUICK3_RND, NITEMS, {
    z_quick3_u64(u64, 0, NITEMS - 1);
  });
  __verify_u64(u64);

  /* quick3 seq */
  for (i = 0; i < NITEMS; ++i) u32[i] = i;
  for (i = 0; i < NITEMS; ++i) u64[i] = i;

  Z_DEBUG_TOPS(u32_QUICK3_SEQ, NITEMS, {
    z_quick3_u32(u32, 0, NITEMS - 1);
  });
  __verify_u32(u32);

  Z_DEBUG_TOPS(u64_QUICK3_RND, NITEMS, {
    z_quick3_u64(u64, 0, NITEMS - 1);
  });
  __verify_u64(u64);

  /* quick3 rev */
  for (i = 0; i < NITEMS; ++i) u32[i] = NITEMS - i;
  for (i = 0; i < NITEMS; ++i) u64[i] = NITEMS - i;

  Z_DEBUG_TOPS(u32_QUICK3_REV, NITEMS, {
    z_quick3_u32(u32, 0, NITEMS - 1);
  });
  __verify_u32(u32);

  Z_DEBUG_TOPS(u64_QUICK3_RND, NITEMS, {
    z_quick3_u64(u64, 0, NITEMS - 1);
  });
  __verify_u64(u64);

  /* heap random */
  seed = NITEMS; for (i = 0; i < NITEMS; ++i) u32[i] = z_bswap32(z_rand32(&seed));
  seed = NITEMS; for (i = 0; i < NITEMS; ++i) u64[i] = z_bswap64(z_rand64(&seed));

  Z_DEBUG_TOPS(u32_HEAP_RND, NITEMS, {
    z_heap_sort_u32(u32, NITEMS);
  });
  __verify_u32(u32);

  Z_DEBUG_TOPS(u64_HEAP_RND, NITEMS, {
    z_heap_sort_u64(u64, NITEMS);
  });
  __verify_u64(u64);

  /* heap seq */
  for (i = 0; i < NITEMS; ++i) u32[i] = i;
  for (i = 0; i < NITEMS; ++i) u64[i] = i;

  Z_DEBUG_TOPS(u32_HEAP_SEQ, NITEMS, {
    z_heap_sort_u32(u32, NITEMS);
  });
  __verify_u32(u32);

  Z_DEBUG_TOPS(u64_HEAP_RND, NITEMS, {
    z_heap_sort_u64(u64, NITEMS);
  });
  __verify_u64(u64);

  /* heap seq */
  for (i = 0; i < NITEMS; ++i) u32[i] = NITEMS - i;
  for (i = 0; i < NITEMS; ++i) u64[i] = NITEMS - i;

  Z_DEBUG_TOPS(u32_HEAP_REV, NITEMS, {
    z_heap_sort_u32(u32, NITEMS);
  });
  __verify_u32(u32);

  Z_DEBUG_TOPS(u64_HEAP_REV, NITEMS, {
    z_heap_sort_u64(u64, NITEMS);
  });
  __verify_u64(u64);

  return 0;
}