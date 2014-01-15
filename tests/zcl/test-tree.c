#include <string.h>
#include <stdio.h>

#include <zcl/global.h>
#include <zcl/test.h>
#include <zcl/tree.h>

struct item {
  z_tree_node_t __node__;
  int key;
};

struct user_data {
  z_tree_node_t *root;
  z_tree_info_t tree_info;
};

static int __item_compare (void *udata, const void *a, const void *b) {
  const struct item *ea = z_container_of(a, const struct item, __node__);
  const struct item *eb = z_container_of(b, const struct item, __node__);
  return(z_cmp(ea->key, eb->key));
}

static int __item_key_compare (void *udata, const void *a, const void *b) {
  const struct item *ea = z_container_of(a, const struct item, __node__);
  return(z_cmp(ea->key, *((int *)b)));
}

static int __test_setup (z_test_t *test) {
  struct user_data *data = (struct user_data *)test->user_data;
  data->root = NULL;

  data->tree_info.plug = &z_tree_avl;
  data->tree_info.node_compare = __item_compare;
  data->tree_info.key_compare = __item_key_compare;
  data->tree_info.node_free = NULL;
  return(0);
}

static int __test_tear_down (z_test_t *test) {
  //struct user_data *data = (struct user_data *)test->user_data;
  return(0);
}

static int __test_iter (z_test_t *test) {
  struct user_data *data = (struct user_data *)test->user_data;
  const z_tree_node_t *node;
  struct item items[8];
  z_tree_iter_t iter;
  int i, nitems;
  int key;

  nitems = sizeof(items) / sizeof(struct item);
  for (i = 0; i < nitems; ++i) {
    items[i].key = i;
    z_tree_node_attach(&(data->tree_info), &(data->root), &(items[i].__node__), NULL);
  }

  z_tree_iter_open(&iter, data->root);

  /* Verify sequential next from first */
  key = 0;
  node = z_tree_iter_begin(&iter);
  while (node != NULL) {
    struct item *pitem = z_container_of(node, struct item, __node__);
    if (pitem->key != key) {
      Z_LOG_ERROR("Failed 1: pitem->key %d != key %d", pitem->key, key);
      return(1);
    }
    node = z_tree_iter_next(&iter);
    key++;
  }
  if (key != nitems) {
    Z_LOG_ERROR("Failed 2: nitems %d != key %d", nitems, key);
    return(2);
  }

  /* Verify sequential next from random key */
  for (i = 0; i < nitems; ++i) {
    key = i;
    node = z_tree_iter_seek_le(&iter, __item_key_compare, &key, NULL);
    while (node != NULL) {
      struct item *pitem = z_container_of(node, struct item, __node__);
      if (pitem->key != key) {
        Z_LOG_ERROR("Failed 3: pitem->key %d != key %d", pitem->key, key);
        return(3);
      }
      node = z_tree_iter_next(&iter);
      key++;
    }
    if (key != nitems) {
      Z_LOG_ERROR("Failed 4: nitems %d != key %d", nitems, key);
      return(4);
    }
  }

  /* Verify sequential next from last */
  key = nitems;
  node = z_tree_iter_end(&iter);
  while (node != NULL) {
    --key;
    struct item *pitem = z_container_of(node, struct item, __node__);
    if (pitem->key != key) {
      Z_LOG_ERROR("Failed 5: pitem->key %d != key %d", pitem->key, key);
      return(1);
    }
    node = z_tree_iter_prev(&iter);
  }
  if (key != 0) {
    Z_LOG_ERROR("Failed 6: 0 != key %d", key);
    return(2);
  }

  /* Verify sequential next from random key */
  for (i = 0; i < nitems; ++i) {
    key = nitems - i;
    --key;
    node = z_tree_iter_seek_le(&iter, __item_key_compare, &key, NULL);
    ++key;
    while (node != NULL) {
      --key;
      struct item *pitem = z_container_of(node, struct item, __node__);
      if (pitem->key != key) {
        Z_LOG_ERROR("Failed 7: pitem->key %d != key %d (i %d)", pitem->key, key, i);
        return(3);
      }
      node = z_tree_iter_prev(&iter);
    }
    if (key != 0) {
      Z_LOG_ERROR("Failed 8: 0 != key %d", nitems, key);
      return(4);
    }
  }

  z_tree_iter_close(&iter);
  return(0);
}

static int __test_iter_seek (z_test_t *test) {
  struct user_data *data = (struct user_data *)test->user_data;
  const z_tree_node_t *node;
  struct item items[8];
  z_tree_iter_t iter;
  struct item *pitem;
  int i, nitems;
  int key;

  nitems = sizeof(items) / sizeof(struct item);
  for (i = 0; i < nitems; ++i) {
    items[i].key = i * 2;
    z_tree_node_attach(&(data->tree_info), &(data->root), &(items[i].__node__), NULL);
  }

  z_tree_iter_open(&iter, data->root);

  for (i = 0; i < nitems; ++i) {
    key = (i % 2) ? i - 1 : i;
    node = z_tree_iter_seek_le(&iter, __item_key_compare, &i, NULL);
    pitem = z_container_of(node, struct item, __node__);
    Z_LOG_INFO("lookup i %d got %d/%d", i, pitem->key, key);
    if (pitem->key != key) {
      Z_LOG_ERROR("Failed 1: pitem->key %d != key %d (i %d)", pitem->key, key, i);
      return(1);
    }
  }


  z_tree_iter_close(&iter);
  return(0);
}

static z_test_t __test_skip_list = {
  .setup      = __test_setup,
  .tear_down  = __test_tear_down,
  .funcs    = {
    __test_iter,
    __test_iter_seek,
    NULL,
  },
};

int main (int argc, char **argv) {
  z_allocator_t allocator;
  struct user_data data;
  int res;

  /* Initialize allocator */
  if (z_system_allocator_open(&allocator))
    return(1);

  /* Initialize global context */
  if (z_global_context_open(&allocator, NULL)) {
    z_allocator_close(&allocator);
    return(1);
  }

  if ((res = z_test_run(&__test_skip_list, &data)))
    printf(" [ !! ] Tree %d\n", res);
  else
    printf(" [ ok ] Tree\n");

  printf("    - z_tree_node_t     %ubytes\n", (unsigned int)sizeof(z_tree_node_t));

  z_global_context_close();
  z_allocator_close(&allocator);
  return(res);
}
