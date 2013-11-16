#include <string.h>
#include <stdio.h>

#include <zcl/global.h>
#include <zcl/test.h>
#include <zcl/tree.h>
#include <zcl/time.h>

#define NUM_ITEMS       3000000
#define NUM_TESTS       3

struct item {
  z_tree_node_t __node__;
  uint64_t key;
};

static int __item_key_compare (void *udata, const void *a, const void *b) {
  uint64_t akey = Z_UINT64_PTR_VALUE(a);
  uint64_t bkey = Z_UINT64_PTR_VALUE(b);
  return(z_cmp(akey, bkey));
}

static struct item *__items_alloc (size_t num_items) {
  struct item *items;
  size_t i;

  items = z_memory_array_alloc(z_global_memory(), struct item, num_items);
  Z_ASSERT(items != NULL, "Expected allocation without failures");

  for (i = 0; i < num_items; ++i) {
    items[i].key = i;
    items[i].__node__.data = &(items[i].key)  ;
  }
  return(items);
}

static void __items_free (struct item *items, size_t num_items) {
  z_memory_array_free(z_global_memory(), items);
}

static void __bench_tree (const z_tree_plug_t *plug, struct item *items, size_t nitems) {
  z_tree_node_t *root = NULL;
  z_tree_info_t tree_info;
  size_t i;

  tree_info.plug = plug;
  tree_info.key_compare = __item_key_compare;
  tree_info.data_free = NULL;
  tree_info.user_data = NULL;

Z_TRACE_TIME(insert_a, {
  for (i = 0; i < nitems; ++i) {
    z_tree_node_attach(&tree_info, &root, Z_TREE_NODE(&(items[i])));
  }
});
Z_LOG_TRACE("Tree Levels: %d (nitems %zu)", z_tree_node_levels(root), i);

Z_TRACE_TIME(detach_min, {
  for (i = 0; i < nitems; ++i) {
    z_tree_node_t *node = z_tree_node_detach_min(&tree_info, &root);
    Z_ASSERT(node == Z_TREE_NODE(&(items[i])),
             "Detach min has detached the wrong node: %p\n", node);
  }
  Z_ASSERT(root == NULL, "Expected an empty tree after items detach-min");
});

Z_TRACE_TIME(insert_b, {
  for (i = 0; i < nitems; ++i) {
    z_tree_node_attach(&tree_info, &root, Z_TREE_NODE(&(items[i])));
  }
});

Z_TRACE_TIME(detach, {
  for (i = 0; i < nitems; ++i) {
    uint64_t key = i;
    z_tree_node_t *node = z_tree_node_detach(&tree_info, &root, &key);
    Z_ASSERT(node == Z_TREE_NODE(&(items[i])),
             "Detach has detached the wrong node: %p\n", node);
  }
  Z_ASSERT(root == NULL, "Expected an empty tree after items detach");
});
}

int main (int argc, char **argv) {
  z_allocator_t allocator;
  struct item *items;
  size_t i;

  /* Initialize allocator */
  if (z_system_allocator_open(&allocator))
    return(1);

  /* Initialize global context */
  if (z_global_context_open(&allocator, NULL)) {
    z_allocator_close(&allocator);
    return(1);
  }

  z_debug_set_log_level(10);

  items = __items_alloc(NUM_ITEMS);

  for (i = 0; i < NUM_TESTS; ++i) {
    Z_LOG_TRACE("AVL");
    Z_LOG_TRACE("------------------------------------------------------------");
    __bench_tree(&z_tree_avl, items, NUM_ITEMS);

    Z_LOG_TRACE("Red-Black");
    Z_LOG_TRACE("------------------------------------------------------------");
    __bench_tree(&z_tree_red_black, items, NUM_ITEMS);
  }

  __items_free(items, NUM_ITEMS);

  z_global_context_close();
  z_allocator_close(&allocator);
  return(0);
}
