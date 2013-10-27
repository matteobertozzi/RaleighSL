#include <zcl/queue.h>
#include <zcl/compression.h>
#include <zcl/allocator.h>
#include <zcl/freelist.h>
#include <zcl/threading.h>
#include <zcl/ringbuf.h>
#include <zcl/queue.h>
#include <zcl/deque.h>
#include <zcl/dlink.h>
#include <zcl/data.h>
#include <zcl/locking.h>
#include <zcl/object.h>
#include <zcl/ticket.h>
#include <zcl/cache.h>
#include <zcl/task.h>
#include <zcl/ipc.h>

#include <stdio.h>

#define __print_size(type)                                \
  printf("%25s %4ld\n", #type, z_sizeof(type));

int main (int argc, char **argv) {
  printf("objects\n");
  printf("---------------------------------\n");
  printf("core/memory\n");
  __print_size(z_free_list_t);
  __print_size(z_allocator_t);
  printf("core/global\n");
  __print_size(z_task_t);
  __print_size(z_task_rwcsem_t);
  __print_size(z_task_queue_t);
  __print_size(z_task_tree_t);
  printf("core/types\n");
  __print_size(z_object_t);
  printf("io/buffers\n");
  __print_size(z_ringbuf_t);
  printf("io/compression\n");
  __print_size(z_compressor_t);
  __print_size(z_decompressor_t);
  printf("data/\n");
  __print_size(z_data_order_t);
  __print_size(z_data_type_t);
  __print_size(z_data_value_t);
  __print_size(z_bytes_t);
  printf("io/ipc\n");
  __print_size(z_ipc_server_t);
  __print_size(z_ipc_client_t);
  __print_size(z_ipc_msgbuf_t);
  printf("threading\n");
  __print_size(z_spinlock_t);
  __print_size(z_ticket_t);
  __print_size(z_rwlock_t);
  __print_size(z_rwcsem_t);
  __print_size(z_mutex_t);
  printf("tools/list\n");
  __print_size(z_dlink_node_t);
  printf("tools/map\n");
  printf("tools/cache\n");
  __print_size(z_cache_entry_t);
  printf("\n");
  printf("vtables\n");
  printf("---------------------------------\n");
  __print_size(z_vtable_type_t);
  __print_size(z_vtable_allocator_t);
  __print_size(z_vtable_queue_t);
  __print_size(z_vtable_deque_t);
  return(0);
}
