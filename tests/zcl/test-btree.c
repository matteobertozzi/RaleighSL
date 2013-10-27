/*
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include <zcl/string.h>
#include <zcl/btree.h>
#include <zcl/debug.h>

static int __node_debug (void *udata, z_btree_item_t *item) {
  char vbuf[512];
  char kbuf[128];

  z_memset(kbuf, '-', item->kprefix);
  z_memcpy(kbuf + item->kprefix, item->key, item->ksize);
  kbuf[item->kprefix + item->ksize] = 0;

  z_memcpy(vbuf, item->value, item->vsize);
  vbuf[item->vsize] = 0;

  fprintf(stderr,
          "Node-Debug: prefix=%u ksize=%u vsize=%u value=\"%s\" key=\"%s\"\n",
          item->kprefix, item->ksize, item->vsize, vbuf, kbuf);
  return(0);
}

#define __node_vtable       z_btree_vnode

#define __node_open(node, size)                               \
  __node_vtable.open(node, size)

#define __node_create(node, size)                             \
  __node_vtable.create(node, size);

#define __node_finalize(node)                                 \
  __node_vtable.finalize(node)

#define __node_fetch_first(node, item)                        \
  __node_vtable.fetch_first(node, item)

#define __node_fetch_next(node, item)                         \
  __node_vtable.fetch_next(node, item)

#define __node_search(node, key, size, item)                  \
  z_btree_node_search(node, &__node_vtable, key, size, item)

#define __node_merge(node_a, node_b, func, udata)             \
  z_btree_node_merge(node_a, &__node_vtable,                  \
                     node_b, &__node_vtable,                  \
                     func, udata)

static void __node_append (uint8_t *block,
                           const void *key, size_t kprefix, size_t ksize,
                           const void *value, size_t vsize)
{
  z_btree_item_t item;
  item.key = key;
  item.value = value;
  item.kprefix = kprefix;
  item.ksize = ksize;
  item.vsize = vsize;
  __node_vtable.append(block, &item);
}

static int __test_lookup (void) {
  z_btree_item_t item;
  uint8_t block[8192];
  int cmp;

  z_memzero(&item, sizeof(z_btree_item_t));
  z_memzero(block, sizeof(block));

  __node_create(block, sizeof(block));
  __node_append(block, "bbbbb", 0, 5, "Value 0", 7);
  __node_append(block,    "cc", 3, 2, "Value 1", 7);
  __node_append(block,   "ccc", 2, 3, "Value 2", 7);
  __node_append(block,    "dd", 3, 2, "Value 3", 7);
  __node_append(block, "ccccc", 0, 5, "Value 4", 7);
  __node_append(block,   "ddd", 2, 3, "Value 5", 7);
  __node_append(block,  "defg", 1, 4, "Value 6", 7);
  __node_append(block, "defgh", 0, 5, "Value 7", 7);
  __node_finalize(block);

  if (__node_open(block, sizeof(block))) {
    fprintf(stderr, "Failed checksum verification\n");
    return(1);
  }

  cmp = __node_search(block, "aaaaa", 5, &item);
  printf("cmp=%d index=%u vsize=%u value=%s\n", cmp, item.index, item.vsize, item.value);
  cmp = __node_search(block, "ccdda", 5, &item);
  printf("cmp=%d index=%u vsize=%u value=%s\n", cmp, item.index, item.vsize, item.value);
  cmp = __node_search(block, "bbbbb", 5, &item);
  printf("cmp=%d index=%u vsize=%u value=%s\n", cmp, item.index, item.vsize, item.value);
  cmp = __node_search(block, "bbbcc", 5, &item);
  printf("cmp=%d index=%u vsize=%u value=%s\n", cmp, item.index, item.vsize, item.value);
  cmp = __node_search(block, "bbccc", 5, &item);
  printf("cmp=%d index=%u vsize=%u value=%s\n", cmp, item.index, item.vsize, item.value);
  cmp = __node_search(block, "bbcdd", 5, &item);
  printf("cmp=%d index=%u vsize=%u value=%s\n", cmp, item.index, item.vsize, item.value);
  cmp = __node_search(block, "ccccc", 5, &item);
  printf("cmp=%d index=%u vsize=%u value=%s\n", cmp, item.index, item.vsize, item.value);
  cmp = __node_search(block, "ccddd", 5, &item);
  printf("cmp=%d index=%u vsize=%u value=%s\n", cmp, item.index, item.vsize, item.value);
  cmp = __node_search(block, "cdefg", 5, &item);
  printf("cmp=%d index=%u vsize=%u value=%s\n", cmp, item.index, item.vsize, item.value);
  cmp = __node_search(block, "defgh", 5, &item);
  printf("cmp=%d index=%u vsize=%u value=%s\n", cmp, item.index, item.vsize, item.value);
  cmp = __node_search(block, "eeeee", 5, &item);
  printf("cmp=%d index=%u vsize=%u value=%s\n", cmp, item.index, item.vsize, item.value);

  return(0);
}

static int __test_merge (void) {
  uint8_t block_a[8192];
  uint8_t block_b[8192];

  z_memzero(block_a, sizeof(block_a));
  __node_create(block_a, sizeof(block_a));

  z_memzero(block_b, sizeof(block_b));
  __node_create(block_b, sizeof(block_b));

  __node_append(block_a, "01131320", 0, 8, "Value 000a", 10);
  __node_append(block_a,  "2312213", 1, 7, "Value 001a", 10);
  __node_append(block_a, "13032222", 0, 8, "Value 002a", 10);
  __node_append(block_a,   "203022", 2, 6, "Value 003a", 10);
  __node_append(block_a, "20020020", 0, 8, "Value 004a", 10);
  __node_append(block_a,   "102201", 2, 6, "Value 005a", 10);
  __node_append(block_a,   "313302", 2, 6, "Value 006a", 10);
  __node_append(block_a,    "20202", 3, 5, "Value 007a", 10);
  __node_append(block_a,  "1000320", 1, 7, "Value 008a", 10);
  __node_append(block_a,   "233033", 2, 6, "Value 009a", 10);
  __node_append(block_a,  "2120203", 1, 7, "Value 010a", 10);
  __node_append(block_a,  "3021232", 1, 7, "Value 011a", 10);
  __node_append(block_a,   "103321", 2, 6, "Value 012a", 10);
  __node_append(block_a,    "32212", 3, 5, "Value 013a", 10);
  __node_append(block_a, "30020111", 0, 8, "Value 014a", 10);
  __node_append(block_a,   "301322", 2, 6, "Value 015a", 10);
  __node_append(block_a,  "1121113", 1, 7, "Value 016a", 10);
  __node_append(block_a,  "3013102", 1, 7, "Value 017a", 10);
  __node_append(block_a,   "330003", 2, 6, "Value 018a", 10);
  __node_append(block_a,     "1301", 4, 4, "Value 019a", 10);


  __node_append(block_b, "02100010", 0, 8, "Value 000b", 10);
  __node_append(block_b,  "3011122", 1, 7, "Value 001b", 10);
  __node_append(block_b,   "102003", 2, 6, "Value 002b", 10);
  __node_append(block_b, "10012310", 0, 8, "Value 003b", 10);
  __node_append(block_b,  "1000220", 1, 7, "Value 004b", 10);
  __node_append(block_b,  "3213333", 1, 7, "Value 005b", 10);
  __node_append(block_b,    "22302", 3, 5, "Value 006b", 10);
  __node_append(block_b,   "301020", 2, 6, "Value 007b", 10);
  __node_append(block_b,      "121", 5, 3, "Value 008b", 10);
  __node_append(block_b, "21032321", 0, 8, "Value 009b", 10);
  __node_append(block_b,   "103131", 2, 6, "Value 010b", 10);
  __node_append(block_b,    "21220", 3, 5, "Value 011b", 10);
  __node_append(block_b,  "3101220", 1, 7, "Value 012b", 10);
  __node_append(block_b,   "220332", 2, 6, "Value 013b", 10);
  __node_append(block_b, "30202111", 0, 8, "Value 014b", 10);
  __node_append(block_b,    "23010", 3, 5, "Value 015b", 10);
  __node_append(block_b,    "31122", 3, 5, "Value 016b", 10);
  __node_append(block_b,  "1231020", 1, 7, "Value 017b", 10);
  __node_append(block_b,  "2011210", 1, 7, "Value 018b", 10);
  __node_append(block_b,   "223331", 2, 6, "Value 019b", 10);

  /*
  __node_append(block_b, "01131320", 0, 8, "Value 000a", 10);
  __node_append(block_b,  "2100010", 1, 7, "Value 000b", 10);
  __node_append(block_b,   "312213", 2, 6, "Value 001a", 10);
  __node_append(block_b,  "3011122", 1, 7, "Value 001b", 10);
  __node_append(block_b,   "102003", 2, 6, "Value 002b", 10);
  __node_append(block_b, "10012310", 0, 8, "Value 003b", 10);
  __node_append(block_b,  "1000220", 1, 7, "Value 004b", 10);
  __node_append(block_b,  "3032222", 1, 7, "Value 002a", 10);
  __node_append(block_b,   "203022", 2, 6, "Value 003a", 10);
  __node_append(block_b,    "13333", 3, 5, "Value 005b", 10);
  __node_append(block_b,    "22302", 3, 5, "Value 006b", 10);
  __node_append(block_b,   "301020", 2, 6, "Value 007b", 10);
  __node_append(block_b,      "121", 5, 3, "Value 008b", 10);
  __node_append(block_b, "20020020", 0, 8, "Value 004a", 10);
  __node_append(block_b,   "102201", 2, 6, "Value 005a", 10);
  __node_append(block_b,   "313302", 2, 6, "Value 006a", 10);
  __node_append(block_b,    "20202", 3, 5, "Value 007a", 10);
  __node_append(block_b,  "1000320", 1, 7, "Value 008a", 10);
  __node_append(block_b,    "32321", 3, 5, "Value 009b", 10);
  __node_append(block_b,   "103131", 2, 6, "Value 010b", 10);
  __node_append(block_b,    "21220", 3, 5, "Value 011b", 10);
  __node_append(block_b,   "233033", 2, 6, "Value 009a", 10);
  __node_append(block_b,  "2120203", 1, 7, "Value 010a", 10);
  __node_append(block_b,  "3021232", 1, 7, "Value 011a", 10);
  __node_append(block_b,   "101220", 2, 6, "Value 012b", 10);
  __node_append(block_b,     "3321", 4, 4, "Value 012a", 10);
  __node_append(block_b,    "32212", 3, 5, "Value 013a", 10);
  __node_append(block_b,   "220332", 2, 6, "Value 013b", 10);
  __node_append(block_b, "30020111", 0, 8, "Value 014a", 10);
  __node_append(block_b,   "202111", 2, 6, "Value 014b", 10);
  __node_append(block_b,    "23010", 3, 5, "Value 015b", 10);
  __node_append(block_b,    "31122", 3, 5, "Value 016b", 10);
  __node_append(block_b,   "301322", 2, 6, "Value 015a", 10);
  __node_append(block_b,  "1121113", 1, 7, "Value 016a", 10);
  __node_append(block_b,   "231020", 2, 6, "Value 017b", 10);
  __node_append(block_b,  "2011210", 1, 7, "Value 018b", 10);
  __node_append(block_b,   "223331", 2, 6, "Value 019b", 10);
  __node_append(block_b,  "3013102", 1, 7, "Value 017a", 10);
  __node_append(block_b,   "330003", 2, 6, "Value 018a", 10);
  __node_append(block_b,     "1301", 4, 4, "Value 019a", 10);
  */

  __node_merge(block_a, block_b, __node_debug, NULL);

  return(0);
}

static int __test_unprefixed (void) {
  z_btree_item_t item;
  uint8_t block[8192];
  int cmp;

  z_memzero(block, sizeof(block));
  __node_create(block, sizeof(block));
  __node_append(block, "aaaaa", 0, 5, "value-00", 8);
  __node_append(block, "abbbb", 0, 5, "value-01", 8);
  __node_append(block, "ccccc", 0, 5, "value-02", 8);
  __node_append(block, "ddddd", 0, 5, "value-03", 8);
  __node_append(block, "ddfff", 0, 5, "value-04", 8);
  __node_append(block, "eefff", 0, 5, "value-05", 8);

  cmp = __node_search(block, "aaaaa", 5, &item);
  printf("cmp=%d index=%u vsize=%u value=%s\n", cmp, item.index, item.vsize, item.value);
  cmp = __node_search(block, "abbbb", 5, &item);
  printf("cmp=%d index=%u vsize=%u value=%s\n", cmp, item.index, item.vsize, item.value);
  cmp = __node_search(block, "ccccc", 5, &item);
  printf("cmp=%d index=%u vsize=%u value=%s\n", cmp, item.index, item.vsize, item.value);
  cmp = __node_search(block, "ddddd", 5, &item);
  printf("cmp=%d index=%u vsize=%u value=%s\n", cmp, item.index, item.vsize, item.value);
  cmp = __node_search(block, "ddfff", 5, &item);
  printf("cmp=%d index=%u vsize=%u value=%s\n", cmp, item.index, item.vsize, item.value);
  cmp = __node_search(block, "eefff", 5, &item);
  printf("cmp=%d index=%u vsize=%u value=%s\n", cmp, item.index, item.vsize, item.value);

  return(0);
}

static int __test_merge_eq (void) {
  uint8_t block_a[8192];
  uint8_t block_b[8192];

  z_memzero(block_a, sizeof(block_a));
  __node_create(block_a, sizeof(block_a));

  z_memzero(block_b, sizeof(block_b));
  __node_create(block_b, sizeof(block_b));

  __node_append(block_a, "00000001", 0, 8, "Value 000a", 10);
  __node_append(block_a,  "1111111", 1, 7, "Value 001a", 10);
  __node_append(block_a,        "4", 7, 1, "Value 002a", 10);

  __node_append(block_b, "00000000", 0, 8, "Value 000b", 10);
  __node_append(block_b,  "1111112", 1, 7, "Value 001b", 10);
  __node_append(block_b,        "3", 7, 1, "Value 002b", 10);

  __node_merge(block_a, block_b, __node_debug, NULL);

  return(0);
}

int main (int argc, char **argv) {
  __test_lookup();
  __test_merge();
  __test_unprefixed();
  __test_merge_eq();
  return(0);
}
