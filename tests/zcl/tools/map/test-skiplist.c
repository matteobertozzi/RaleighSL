/*
 *   Copyright 2011-2012 Matteo Bertozzi
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

#include <zcl/skiplist.h>
#include <zcl/string.h>
#include <zcl/test.h>
#include <zcl/map.h>

static int __key_compare (void *udata, const void *a, const void *b) {
    return(z_strcmp((const char *)a, (const char *)b));
}

static void __dump (const void *data) {
    printf("%p: %s\n", data, (const char *)data);
}

static int __test_map (void *self) {
    z_memory_t memory;
    z_skip_list_t map;
    void *p;

    z_system_memory_init(&memory);
    z_skip_list_alloc(&map, &memory, __key_compare, NULL, NULL, 1);

    printf("Type Name: %s\n", z_type_name(&map));
    printf("Type Size: %u\n", z_type_size(&map));
    printf("Object Size: %lu\n", sizeof(z_object_t));
    z_map_put(&map, "abba");
    z_map_put(&map, "roba");
    z_map_put(&map, "nola");
    z_map_put(&map, "aaaa");
    p = z_map_get(&map, "abba"); __dump(p);
    p = z_map_get(&map, "nola"); __dump(p);
    p = z_map_get(&map, "roba"); __dump(p);
    p = z_map_get(&map, "aaaa"); __dump(p);
    z_map_remove(&map, "nola");
    p = z_map_get(&map, "abba"); __dump(p);
    p = z_map_get(&map, "nola"); __dump(p);
    p = z_map_get(&map, "roba"); __dump(p);
    p = z_map_get(&map, "aaaa"); __dump(p);

    printf("ITER\n");
    z_skip_list_iterator_t iter;
    z_iterator_open(&iter, &map);
    p = z_iterator_end(&iter);
    do {
        __dump(p);
    } while ((p = z_iterator_prev(&iter)) != NULL);
    z_iterator_close(&iter);

    z_skip_list_free(&map);
    return(0);
}

static const z_test_t __test_skiplist = {
    .name       = "Test SkipList",
    .setup      = NULL,
    .tear_down  = NULL,
    .funcs      = {
        __test_map,
        NULL,
    },
};

int main (int argc, char **argv) {
    return(z_test_run(&__test_skiplist, NULL));
}