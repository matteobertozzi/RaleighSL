#include <zcl/vtask.h>

#define __NTASKS      32

int main (int argc, char **argv) {
  z_vtask_t tasks[__NTASKS];
  z_vtask_tree_t tree;
  z_vtask_t *p;
  int i;

  for (i = 0; i < __NTASKS; ++i) {
    z_vtask_reset(tasks + i, Z_VTASK_TYPE_TASK);
    (tasks + i)->seqid = 1 + i;
  }

  z_vtask_tree_init(&tree);
  z_vtask_tree_push(&tree, tasks + 16);
  fprintf(stdout, "MIN %"PRIu64"\n", tree.min_node->seqid);
  z_vtask_tree_push(&tree, tasks + 17);
  fprintf(stdout, "MIN %"PRIu64"\n", tree.min_node->seqid);
  z_vtask_tree_push(&tree, tasks + 15);
  fprintf(stdout, "MIN %"PRIu64"\n", tree.min_node->seqid);
  while (tree.min_node != NULL) {
    p = z_vtask_tree_pop(&tree);
    z_vtask_tree_dump(&tree, stdout);
    fprintf(stdout, "MIN %"PRIu64" POP %"PRIu64"\n",
                    (tree.min_node ? tree.min_node->seqid : 0), p->seqid);
  }
  z_vtask_tree_dump(&tree, stdout);

  for (i = 0; i < __NTASKS; ++i) {
    z_vtask_tree_push(&tree, tasks + i);
    z_vtask_tree_dump(&tree, stdout);
  }

  for (i = 0; i < __NTASKS; ++i) {
    p = z_vtask_tree_pop(&tree);
    fprintf(stdout, "MIN %"PRIu64" POP %"PRIu64"\n",
      (tree.min_node ? tree.min_node->seqid : 0), p->seqid);
    z_vtask_tree_dump(&tree, stdout);
  }

  for (i = 0; i < __NTASKS; ++i) {
    z_vtask_tree_push(&tree, tasks + (__NTASKS - 1) - i);
    z_vtask_tree_dump(&tree, stdout);
  }

  for (i = 0; i < __NTASKS; ++i) {
    p = z_vtask_tree_pop(&tree);
    fprintf(stdout, "MIN %"PRIu64" POP %"PRIu64"\n",
      (tree.min_node ? tree.min_node->seqid : 0), p->seqid);
    z_vtask_tree_dump(&tree, stdout);
  }

  return(0);
}
