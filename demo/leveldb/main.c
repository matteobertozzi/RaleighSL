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

#include <stdio.h>

#include <zcl/iopoll.h>
#include <zcl/rpc.h>

#include "zleveldb.h"

#define DEFAULT_CACHE_SIZE         100000

static const char *DEFAULT_PORT = "11213";

extern z_rpc_protocol_t leveldb_protocol;

int main (int argc, char **argv) {
    z_leveldb_t leveldb;
    z_iopoll_t iopoll;
    z_memory_t memory;
    const char *port;

    if (argc < 2) {
        printf("usage: demo-leveldb <filename> [port]\n");
        return(1);
    }
    
    port = (argc > 2) ? argv[2] : DEFAULT_PORT;

    if (z_leveldb_open(&leveldb, argv[1], DEFAULT_CACHE_SIZE))
        return(1);

    z_memory_init(&memory, z_system_allocator());
    z_iopoll_alloc(&iopoll, &memory, NULL, 0, NULL);
    z_rpc_plug(&memory, &iopoll, &leveldb_protocol, NULL, port, &leveldb);
    z_iopoll_loop(&iopoll);
    z_iopoll_free(&iopoll);
    z_leveldb_close(&leveldb);

    return(0);
}

