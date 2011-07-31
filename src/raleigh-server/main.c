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

#include <zcl/messageq.h>
#include <zcl/iopoll.h>
#include <zcl/memcpy.h>
#include <zcl/rpc.h>

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "engine.h"

extern z_rpc_protocol_t memcache_protocol;
extern z_rpc_protocol_t redis_protocol;

static void __request_exec (void *user_data, z_message_t *msg) {
    storage_request_exec((struct storage *)user_data, msg);
}

int main (int argc, char **argv) {
    z_messageq_t messageq;
    struct storage engine;
    z_iopoll_t iopoll;
    z_memory_t memory;

    z_memory_init(&memory, z_system_allocator());

    storage_alloc(&engine, &memory);

    z_iopoll_alloc(&iopoll, &memory, NULL, 0, NULL);
    z_messageq_alloc(&messageq, &memory, &z_messageq_noop,
                     __request_exec, &engine);

    z_rpc_plug(&memory, &iopoll, &redis_protocol, NULL, "11214", &messageq);
    z_rpc_plug(&memory, &iopoll, &memcache_protocol, NULL, "11212", &messageq);

    z_iopoll_loop(&iopoll);
    z_iopoll_free(&iopoll);
    z_messageq_free(&messageq);

    return(0);
}
