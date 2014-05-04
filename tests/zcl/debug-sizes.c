#include <zcl/histogram.h>
#include <zcl/dbuffer.h>
#include <zcl/global.h>
#include <zcl/iopoll.h>

#include <zcl/spinlock.h>
#include <zcl/waitcond.h>
#include <zcl/thread.h>
#include <zcl/mutex.h>

#include <zcl/threadpool.h>
#include <zcl/semaphore.h>
#include <zcl/rwlock.h>
#include <zcl/rwcsem.h>
#include <zcl/ticket.h>

#include <zcl/task.h>
#include <zcl/ipc.h>
#include <zcl/rpc.h>

#include <zcl/stailq.h>
#include <zcl/slink.h>
#include <zcl/dlink.h>
#include <zcl/chmap.h>

#include <stdio.h>

#define __print_size(type)                                \
  printf("%25s %4ld\n", #type, z_sizeof(type))

int main (int argc, char **argv) {
  printf("core/memory\n");
  __print_size(z_allocator_t);
  __print_size(z_memory_t);

  printf("data\n");
  __print_size(z_dbuf_node_t);
  __print_size(z_dbuf_writer_t);
  __print_size(z_dbuf_reader_t);

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
  __print_size(z_task_rq_stats_t);

  printf("io/ipc\n");
  __print_size(z_ipc_server_t);
  __print_size(z_ipc_client_t);
  __print_size(z_ipc_msgbuf_t);
  __print_size(z_ipc_msg_t);
  __print_size(z_ipc_msg_client_t);

  printf("io/poll\n");
  __print_size(z_iopoll_stats_t);
  __print_size(z_iopoll_entity_t);

  printf("io/rpc\n");
  __print_size(z_rpc_ctx_t);
  __print_size(z_rpc_task_t);
  __print_size(z_rpc_call_t);
  __print_size(z_rpc_map_t);

  printf("metrics\n");
  __print_size(z_histogram_t);

  printf("system\n");
  __print_size(z_mutex_t);
  __print_size(z_wait_cond_t);
  __print_size(z_spinlock_t);
  __print_size(z_thread_t);

  printf("threading\n");
  __print_size(z_rwcsem_t);
  __print_size(z_rwlock_t);
  __print_size(z_semaphore_t);
  __print_size(z_thread_pool_t);
  __print_size(z_ticket_t);

  printf("util/list\n");
  __print_size(z_slink_node_t);
  __print_size(z_dlink_node_t);
  __print_size(z_stailq_t);

  printf("util/map\n");
  __print_size(z_chmap_entry_t);

  return(0);
}