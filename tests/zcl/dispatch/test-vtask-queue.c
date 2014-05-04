#include <zcl/vtask.h>

#define __NTASKS  8

int main (int argc, char **argv) {
  z_vtask_t vtasks[__NTASKS];
  z_vtask_queue_t queue;
  int i;

  for (i = 0; i < __NTASKS; ++i) {
    vtasks[i].vtime = 0;
    vtasks[i].seqid = i + 1;
  }

  z_vtask_queue_init(&queue);
  for (i = 0; i < __NTASKS; ++i) {
    z_vtask_queue_push(&queue, vtasks + i);
    z_vtask_queue_dump(&queue, stdout);
  }
  fprintf(stderr, "END-INSERT\n");
#if 1
  for (i = 0; i < __NTASKS; i += 2) {
    z_vtask_queue_remove(&queue, vtasks + i);
    z_vtask_queue_dump(&queue, stdout);
  }
  for (i = 1; i < __NTASKS; i += 2) {
    z_vtask_queue_remove(&queue, vtasks + i);
    z_vtask_queue_dump(&queue, stdout);
  }
#else
  z_vtask_queue_remove(&queue, vtasks + __NTASKS - 1);
  z_vtask_queue_dump(&queue, stdout);
#endif
  fprintf(stderr, "END-REMOVE\n");

  return(0);
}
