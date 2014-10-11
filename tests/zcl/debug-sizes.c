#include <zcl/allocator.h>
#include <zcl/array.h>
#include <zcl/humans.h>
#include <zcl/iopoll.h>
#include <zcl/memory.h>
#include <zcl/ipc.h>
#include <stdio.h>

#define __print_size(type)                          \
  do {                                              \
    char buffer[32];                                \
    z_human_size(buffer, 32, sizeof(type));         \
    printf("%25s %10s\n", #type, buffer);           \
  } while (0)

int main (int argc, char **argv) {
  printf("core/memory\n");
  __print_size(z_allocator_t);
  __print_size(z_memory_t);

  printf("io/poll\n");
  __print_size(z_iopoll_entity_t);
  __print_size(z_iopoll_stats_t);

  printf("io/ipc\n");
  __print_size(z_ipc_server_t);
  __print_size(z_ipc_client_t);

  printf("io/ipc-msg\n");
  __print_size(z_ipc_msgbuf_t);
  __print_size(z_ipc_msg_t);
  __print_size(z_ipc_msg_client_t);

  printf("util/array\n");
  __print_size(z_array_t);

  return(0);
}
