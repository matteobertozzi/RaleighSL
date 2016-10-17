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

#include <zcl/histogram.h>
#include <zcl/avl-link.h>
#include <zcl/rblink.h>
#include <zcl/humans.h>
#include <zcl/memory.h>
#include <zcl/iopoll.h>
#include <zcl/sched.h>
#include <zcl/dlink.h>
#include <zcl/codel.h>
#include <zcl/shmem.h>
#include <zcl/ipc-msg.h>
#include <zcl/ipc.h>

#include <stdio.h>

#define __print_size(type)                                              \
  do {                                                                  \
    char buffer[32];                                                    \
    z_human_size(buffer, 32, sizeof(type));                             \
    printf("%25s %10s (%5d) %6.2f 64s\n",                               \
           #type, buffer, (int)sizeof(type), (float)sizeof(type)/64);   \
  } while (0)

int main (int argc, char **argv) {
  printf("core/memory\n");
  __print_size(z_allocator_t);
  __print_size(z_memory_t);

  printf("eloop/ipc\n");
  __print_size(z_ipc_server_t);
  __print_size(z_ipc_client_t);
  __print_size(z_ipc_msg_stats_t);
  __print_size(z_ipc_msg_head_t);
  __print_size(z_ipc_msg_buf_t);
  __print_size(z_ipc_msg_reader_t);
  __print_size(z_ipc_msg_writer_t);

  printf("eloop/iopoll\n");
  __print_size(z_iopoll_entity_t);
  __print_size(z_iopoll_load_t);
  __print_size(z_iopoll_stats_t);
  __print_size(z_iopoll_engine_t);

  printf("eloop/sched\n");
  __print_size(z_sched_task_t);
  __print_size(z_sched_queue_t);
  __print_size(z_sched_t);

  printf("tracing\n");
  __print_size(z_histogram_t);

  printf("system\n");
  __print_size(z_shmem_t);

  printf("util/algo\n");
  __print_size(z_codel_t);

  printf("util/link\n");
  __print_size(z_dlink_node_t);
  __print_size(z_rblink_t);
  __print_size(z_rbtree_t);
  __print_size(z_rb_iter_t);
  __print_size(z_avl_link_t);
  __print_size(z_avl_tree_t);
  __print_size(z_avl_iter_t);

  return(0);
}
