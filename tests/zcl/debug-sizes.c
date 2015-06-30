#include <zcl/histogram.h>
#include <zcl/allocator.h>
#include <zcl/eloop.h>
#include <zcl/humans.h>
#include <zcl/ipc.h>
#include <zcl/iopoll.h>
#include <zcl/memory.h>
#include <zcl/thread.h>
#include <zcl/waitcond.h>
#include <zcl/mutex.h>
#include <zcl/spinlock.h>
#include <zcl/avl16.h>
#include <zcl/msg.h>
#include <stdio.h>

#define __print_size(type)                                              \
  do {                                                                  \
    char buffer[32];                                                    \
    z_human_size(buffer, 32, sizeof(type));                             \
    printf("%25s %10s (%5d) %6.2f 64s\n",                               \
           #type, buffer, (int)sizeof(type), (float)sizeof(type)/64);   \
  } while (0)

int main (int argc, char **argv) {
  printf("eloop\n");
  __print_size(z_event_loop_t);


  printf("eloop/poll\n");
  __print_size(z_iopoll_entity_t);
  __print_size(z_iopoll_stats_t);
  __print_size(z_iopoll_engine_t);

  printf("io/ipc\n");
  __print_size(z_ipc_server_t);
  __print_size(z_ipc_client_t);
  __print_size(z_ipc_msg_client_t);

  printf("io/msg\n");
  __print_size(z_msg_ibuf_t);

  printf("core/memory\n");
  __print_size(z_allocator_t);
  __print_size(z_memory_t);

  printf("system\n");
  __print_size(z_thread_t);
  __print_size(z_mutex_t);
  __print_size(z_spinlock_t);
  __print_size(z_wait_cond_t);

  printf("metrics\n");
  __print_size(z_histogram_t);

  printf("util/sset\n");
  __print_size(z_avl16_node_t);
  __print_size(z_avl16_iter_t);

  return(0);
}
