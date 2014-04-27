#include <zcl/vtask.h>

int main (int argc, char **argv) {
  z_vtask_t tasks[512];
  z_vtask_tree_t tree;
  z_vtask_t *p;
  int i;

  for (i = 0; i < 512; ++i) {
    z_vtask_reset(tasks + i, Z_VTASK_TYPE_TASK);
    (tasks + i)->seqid = i;
  }

  z_vtask_tree_init(&tree);
  z_vtask_tree_push(&tree, tasks + 16);
  fprintf(stderr, "MIN %"PRIu64"\n", tree.min_node->seqid);
  z_vtask_tree_push(&tree, tasks + 17);
  fprintf(stderr, "MIN %"PRIu64"\n", tree.min_node->seqid);
  z_vtask_tree_push(&tree, tasks + 15);
  fprintf(stderr, "MIN %"PRIu64"\n", tree.min_node->seqid);
  p = z_vtask_tree_pop(&tree);
  fprintf(stderr, "MIN %"PRIu64" POP %"PRIu64"\n", tree.min_node->seqid, p->seqid);
  p = z_vtask_tree_pop(&tree);
  fprintf(stderr, "MIN %"PRIu64" POP %"PRIu64"\n", tree.min_node->seqid, p->seqid);
  p = z_vtask_tree_pop(&tree);
  fprintf(stderr, "MIN %p POP %"PRIu64" ROOT %p\n", tree.min_node, p->seqid, tree.root);

  for (i = 0; i < 512; ++i) {
    z_vtask_tree_push(&tree, tasks + i);
  }

  for (i = 0; i < 512; ++i) {
    p = z_vtask_tree_pop(&tree);
    fprintf(stderr, "MIN %"PRIu64" POP %"PRIu64"\n",
      (tree.min_node ? tree.min_node->seqid : 0), p->seqid);
  }

  for (i = 0; i < 512; ++i) {
    z_vtask_tree_push(&tree, tasks + 511 - i);
  }

  for (i = 0; i < 512; ++i) {
    p = z_vtask_tree_pop(&tree);
    fprintf(stderr, "MIN %"PRIu64" POP %"PRIu64"\n",
      (tree.min_node ? tree.min_node->seqid : 0), p->seqid);
  }

  return(0);
}
