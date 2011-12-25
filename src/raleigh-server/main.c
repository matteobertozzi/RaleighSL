/*
 *   Copyright 2011 Matteo Bertozzi
 *
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

#include <raleighfs/object/memcache.h>
#include <raleighfs/object/counter.h>
#include <raleighfs/object/deque.h>
#include <raleighfs/object/sset.h>

#include <raleighfs/semantic/flat.h>
#include <raleighfs/device/memory.h>
#include <raleighfs/key/flat.h>

#include <raleighfs/raleighfs.h>

#include <zcl/messageq.h>
#include <zcl/iopoll.h>
#include <zcl/memcpy.h>
#include <zcl/rpc.h>

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

extern z_rpc_protocol_t raleigh_v1_protocol;
extern z_rpc_protocol_t memcache_protocol;
extern z_rpc_protocol_t redis_protocol;

static int __is_running = 1;
static void __signal_handler (int signum) {
    __is_running = 0;
}

static void __request_exec (void *user_data, z_message_t *msg) {
    raleighfs_execute(RALEIGHFS(user_data), msg);
}

static int __init_static_objects (raleighfs_t *fs) {
    z_rdata_t name;

    z_rdata_from_blob(&name, ":memcache:", 10);
    raleighfs_semantic_touch(fs, &raleighfs_object_memcache, &name);

    return(0);
}

int main (int argc, char **argv) {
    raleighfs_device_t device;
    raleighfs_errno_t errno;
    z_messageq_t messageq;
    z_memory_t memory;
    z_iopoll_t iopoll;
    raleighfs_t fs;

    signal(SIGINT, __signal_handler);

    z_memory_init(&memory, z_system_allocator());

    if (raleighfs_alloc(&fs, &memory) == NULL)
        return(1);

    /* Plug semantic layer */
    raleighfs_plug_semantic(&fs, &raleighfs_semantic_flat);

    /* Plug objects */
    raleighfs_plug_object(&fs, &raleighfs_object_memcache);
    raleighfs_plug_object(&fs, &raleighfs_object_counter);
    raleighfs_plug_object(&fs, &raleighfs_object_deque);
    raleighfs_plug_object(&fs, &raleighfs_object_sset);

    raleighfs_memory_device(&device);
    if ((errno = raleighfs_create(&fs, &device,
                                  &raleighfs_key_flat,
                                  NULL, NULL,
                                  &raleighfs_semantic_flat)))
    {
        fprintf(stderr, "raleighfs_create(): %s\n",
                raleighfs_errno_string(errno));
        return(2);
    }

    __init_static_objects(&fs);

    z_iopoll_alloc(&iopoll, &memory, NULL, 0, &__is_running);
    z_messageq_alloc(&messageq, &memory, &z_messageq_noop,
                     __request_exec, &fs);

    z_rpc_plug(&memory, &iopoll, &raleigh_v1_protocol, NULL,"11215", &messageq);
    z_rpc_plug(&memory, &iopoll, &redis_protocol, NULL, "11214", &messageq);
    z_rpc_plug(&memory, &iopoll, &memcache_protocol, NULL, "11212", &messageq);

    z_iopoll_loop(&iopoll);
    z_iopoll_free(&iopoll);
    z_messageq_free(&messageq);
    raleighfs_close(&fs);
    raleighfs_free(&fs);

    return(0);
}
