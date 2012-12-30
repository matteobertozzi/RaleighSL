#include <zcl/bytes.h>
#include <zcl/slice.h>
#include <zcl/iobuf.h>
#include <zcl/ipc.h>
#include <zcl/iopoll.h>
#include <zcl/skiplist.h>
#include <zcl/linked-deque.h>
#include <zcl/linked-queue.h>
#include <zcl/extent.h>
#include <zcl/time.h>

#include <stdio.h>

#define __sizeof64(x)       ((uint64_t)sizeof(x))

int main (int argc, char **argv) {
    printf("zcl/buffers\n");
    printf("  z_bytes_t          %lu\n", __sizeof64(z_bytes_t));
    printf("  z_byteslice_t      %lu\n", __sizeof64(z_byteslice_t));
    printf("  z_slice_node_t     %lu\n", __sizeof64(z_slice_node_t));
    printf("  z_slice_t          %lu\n", __sizeof64(z_slice_t));
    printf("zcl/core/memory\n");
    printf("  z_allocator_t      %lu\n", __sizeof64(z_allocator_t));
    printf("  z_memory_t         %lu\n", __sizeof64(z_memory_t));
    printf("zcl/core/types\n");
    printf("  z_object_t         %lu\n", __sizeof64(z_object_t));
    printf("zcl/core/types\n");
    printf("  z_opaque_t         %lu\n", __sizeof64(z_opaque_t));
    printf("  z_timer_t          %lu\n", __sizeof64(z_timer_t));
    printf("zcl/io/buffer\n");
    printf("  z_iobuf_t          %lu\n", __sizeof64(z_iobuf_t));
    printf("zcl/io/ipc\n");
    printf("  z_ipc_server_t     %lu\n", __sizeof64(z_ipc_server_t));
    printf("  z_ipc_client_t     %lu\n", __sizeof64(z_ipc_client_t));
    printf("  z_ipc_msgbuf_t     %lu\n", __sizeof64(z_ipc_msgbuf_t));
    printf("  z_ipc_msg_t        %lu\n", __sizeof64(z_ipc_msg_t));
    printf("zcl/io/poll\n");
    printf("  z_iopoll_entity_t  %lu\n", __sizeof64(z_iopoll_entity_t));
    printf("  z_iopoll_t         %lu\n", __sizeof64(z_iopoll_t));
    printf("zcl/tool/map\n");
    printf("  z_skip_list_t      %lu\n", __sizeof64(z_skip_list_t));
    printf("zcl/tool/queue\n");
    printf("  z_linked_deque_t   %lu\n", __sizeof64(z_linked_deque_t));
    printf("  z_linked_queue_t   %lu\n", __sizeof64(z_linked_queue_t));
    printf("zcl\n");
    printf("  z_extent_t         %lu\n", __sizeof64(z_extent_t));
    return(0);
}