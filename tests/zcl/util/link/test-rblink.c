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

#include <zcl/utest.h>
#include <zcl/rblink.h>

struct object_node {
  z_rblink_t link;
  int id;
};

static int __object_node_cmp (void *udata, const z_rblink_t *a, const z_rblink_t *b) {
  const struct object_node *node_a = z_container_of(a, struct object_node, link);
  const struct object_node *node_b = z_container_of(b, struct object_node, link);
  return(node_a->id - node_b->id);
}

static int __object_key_cmp (void *udata, const void *key, const z_rblink_t *x) {
  const struct object_node *node_x = z_container_of(x, struct object_node, link);
  const int *k = Z_CAST(int, key);
  return(*k - node_x->id);
}

static void test_crud (z_utest_env_t *env) {
  struct object_node nodes[5];
  z_rblink_t *node;
  z_rbtree_t tree;
  int i, j;

  z_rb_tree_init(&tree);
  for (i = 4; i >= 0; --i) {
    nodes[i].id = i + 1;
    z_rb_tree_insert(&tree, __object_node_cmp, &(nodes[i].link), NULL);
    for (j = 5; j > i; --j) {
      struct object_node *obj;
      node = z_rb_tree_lookup(&tree, __object_key_cmp, &j, NULL);
      z_assert_not_null(env, node);
      obj = z_container_of(node, struct object_node, link);
      z_assert_int_equals(env, j, obj->id);
    }
  }

  i = 0;
  node = z_rb_tree_lookup(&tree, __object_key_cmp, &i, NULL);
  z_assert_null(env, node);
  for (i = 1; i < 6; ++i) {
    struct object_node *obj;
    node = z_rb_tree_lookup(&tree, __object_key_cmp, &i, NULL);
    z_assert_not_null(env, node);
    obj = z_container_of(node, struct object_node, link);
    z_assert_int_equals(env, i, obj->id);
  }
  node = z_rb_tree_lookup(&tree, __object_key_cmp, &i, NULL);
  z_assert_null(env, node);

  for (i = 1; i <= 5; ++i) {
    z_rb_tree_remove(&tree, __object_key_cmp, &i, NULL);
    for (j = 1; j <= i; ++j) {
      node = z_rb_tree_lookup(&tree, __object_key_cmp, &j, NULL);
      z_assert_null(env, node);
    }
    for (; j <= 5; ++j) {
      struct object_node *obj;
      node = z_rb_tree_lookup(&tree, __object_key_cmp, &j, NULL);
      z_assert_not_null(env, node);
      obj = z_container_of(node, struct object_node, link);
      z_assert_int_equals(env, j, obj->id);
    }
  }
}

static void test_iter (z_utest_env_t *env) {
  struct object_node nodes[5];
  z_rblink_t *node;
  z_rbtree_t tree;
  z_rb_iter_t iter;
  int i;

  z_rb_tree_init(&tree);
  for (i = 0; i < 10; i += 2) {
    nodes[i].id = i;
    z_rb_tree_insert(&tree, __object_node_cmp, &(nodes[i].link), NULL);
  }

  z_rb_iter_init(&iter, &tree);
  node = z_rb_iter_seek_begin(&iter);
  for (i = 0; node != NULL; i += 2) {
    struct object_node *obj = z_container_of(node, struct object_node, link);
    z_assert_int_equals(env, i, obj->id);
    node = z_rb_iter_next(&iter);
  }

  node = z_rb_iter_seek_end(&iter);
  for (i = 8; node != NULL; i -= 2) {
    struct object_node *obj = z_container_of(node, struct object_node, link);
    z_assert_int_equals(env, i, obj->id);
    node = z_rb_iter_prev(&iter);
  }

  i = 3;
  node = z_rb_iter_seek_le(&iter, __object_key_cmp, &i, NULL);
  for (--i; node != NULL; i += 2) {
    struct object_node *obj = z_container_of(node, struct object_node, link);
    z_assert_int_equals(env, i, obj->id);
    node = z_rb_iter_next(&iter);
  }

  i = 3;
  node = z_rb_iter_seek_ge(&iter, __object_key_cmp, &i, NULL);
  for (++i; node != NULL; i += 2) {
    struct object_node *obj = z_container_of(node, struct object_node, link);
    z_assert_int_equals(env, i, obj->id);
    node = z_rb_iter_next(&iter);
  }
}

int main (int argc, char **argv) {
  int r = 0;
  r |= z_utest_run(test_crud, 60000);
  r |= z_utest_run(test_iter, 60000);
  return(r);
}
