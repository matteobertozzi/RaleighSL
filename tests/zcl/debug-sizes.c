#include <zcl/global.h>
#include <zcl/task.h>
#include <stdio.h>

#define __print_size(type)                                \
  printf("%25s %4ld\n", #type, z_sizeof(type))

int main (int argc, char **argv) {
  printf("core/memory\n");
  __print_size(z_allocator_t);
  __print_size(z_memory_t);

  printf("dispatch\n");
  __print_size(z_vtask_t);
  __print_size(z_vtask_tree_t);
  __print_size(z_vtask_queue_t);
  __print_size(z_task_t);
  __print_size(z_task_rq_t);
  __print_size(z_task_rq_rr_t);
  __print_size(z_task_rq_fair_t);
  __print_size(z_task_rq_fifo_t);
  __print_size(z_task_rq_group_t);
  return(0);
}