#include <string.h>
#include <stdio.h>

#include <zcl/chunkq.h>
#include <zcl/string.h>
#include <zcl/memcmp.h>
#include <zcl/test.h>

struct user_data {
    z_memory_t  memory;
    z_chunkq_t  chunkq;
};

static int __test_setup (z_test_t *test) {
    struct user_data *data = (struct user_data *)(test->user_data);
    z_chunkq_alloc(&(data->chunkq), &(data->memory), 16);
    return(0);
}

static int __test_tear_down (z_test_t *test) {
    struct user_data *data = (struct user_data *)(test->user_data);
    z_chunkq_free(&(data->chunkq));
    return(0);
}

static int __test_crud (z_test_t *test) {
    struct user_data *data = (struct user_data *)(test->user_data);
    z_chunkq_append(&(data->chunkq), "echo\r\n", 6);
    if (z_chunkq_indexof(&(data->chunkq), 0, "echo", 4))
        return(1);
    z_chunkq_remove(&(data->chunkq), 6);
    z_chunkq_append(&(data->chunkq), "echo\r\n", 6);
    if (z_chunkq_indexof(&(data->chunkq), 0, "echo", 4))
        return(2);

    if (z_chunkq_memcmp(&(data->chunkq), 0, "echo\r\n", 6))
        return(3);

    return(0);
}

static int __test_nblocks (z_test_t *test) {
    struct user_data *data = (struct user_data *)(test->user_data);
    z_chunkq_t *cq = &(data->chunkq);
    char buffer[64];

    z_chunkq_append(cq, "Licensed to the Apache Software Foundation\r\n", 44);
    z_chunkq_append(cq, "The U.S. Government Department of Commerce\r\n", 44);
    z_chunkq_append(cq, "For the latest information visit our website\n", 45);

    if (!z_chunkq_startswith(cq, 0, "Licensed to the Apache Software Foundation\r\n", 44))
        return(1);

    if (!z_chunkq_startswith(cq, 44, "The U.S. Government Department of Commerce\r\n", 44))
        return(2);

    if (!z_chunkq_startswith(cq, 88, "For the latest information visit our website\n", 45))
        return(3);

    if (z_chunkq_pop(cq, buffer, 16) != 16)
        return(4);

    if (z_memcmp(buffer, "Licensed to the ", 16))
        return(5);

    if (!z_chunkq_startswith(cq, 0, "Apache Software Foundation", 26))
        return(6);

    if (z_chunkq_pop(cq, buffer, 15) != 15)
        return(7);

    if (z_memcmp(buffer, "Apache Software", 15))
        return(8);

    if (!z_chunkq_startswith(cq, 1, "Foundation", 10))
        return(9);

    if (!z_chunkq_startswith(cq, 0, " Foundation", 11))
        return(9);

    /* There was a bug here */
    z_chunkq_remove(cq, 11);
    //z_chunkq_pop(cq, buffer, 11);
    if (!z_chunkq_startswith(cq, 0, "\r\n", 2))
        return(11);

    z_chunkq_remove(cq, 2);
    if (!z_chunkq_startswith(cq, 0, "The U.S. Government", 19))
        return(13);

    return(0);
}

static int __test_tokenize (z_test_t *test) {
    struct user_data *data = (struct user_data *)(test->user_data);
    z_chunkq_t *cq = &(data->chunkq);
    z_chunkq_extent_t extent;
    unsigned int off;

    z_chunkq_append(cq, " \t this is a    test!\n tokenize me!\n bye", 40);

    off = 0;
    if (!z_chunkq_tokenize(cq, &extent, off, " \n\t", 3))
        return(1);

    if (extent.length != 4 || z_chunkq_memcmp(cq, extent.offset, "this", 4))
        return(2);

    off = extent.offset + extent.length;
    if (!z_chunkq_tokenize(cq, &extent, off, " \n\t", 3))
        return(3);

    if (extent.length != 2 || z_chunkq_memcmp(cq, extent.offset, "is", 2))
        return(4);

    off = extent.offset + extent.length;
    if (!z_chunkq_tokenize(cq, &extent, off, " \n\t", 3))
        return(5);

    if (extent.length != 1 || z_chunkq_memcmp(cq, extent.offset, "a", 1))
        return(6);

    off = extent.offset + extent.length;
    if (!z_chunkq_tokenize(cq, &extent, off, " \n\t", 3))
        return(7);

    if (extent.length != 5 || z_chunkq_memcmp(cq, extent.offset, "test!", 5))
        return(8);

    off = extent.offset + extent.length;
    if (!z_chunkq_tokenize(cq, &extent, off, " \n\t", 3))
        return(9);

    if (extent.length != 8 || z_chunkq_memcmp(cq, extent.offset, "tokenize", 8))
        return(10);

    off = extent.offset + extent.length;
    if (!z_chunkq_tokenize(cq, &extent, off, " \n\t", 3))
        return(11);

    if (extent.length != 3 || z_chunkq_memcmp(cq, extent.offset, "me!", 3))
        return(12);

    off = extent.offset + extent.length;
    if (!z_chunkq_tokenize(cq, &extent, off, " \n\t", 3))
        return(13);

    if (extent.length != 3 || z_chunkq_memcmp(cq, extent.offset, "bye", 3))
        return(14);

    off = extent.offset + extent.length;
    if (z_chunkq_tokenize(cq, &extent, off, " \n\t", 3))
        return(15);

    return(0);
}

static int __test_memcmp (z_test_t *test) {
    struct user_data *data = (struct user_data *)(test->user_data);
    z_chunkq_t *cq = &(data->chunkq);

    z_chunkq_append(cq, "Licensed to the Apache Software Foundation\r\n", 44);

    if (z_chunkq_memcmp(cq, 0, "Licensed", 8))
        return(1);

    if (z_chunkq_memcmp(cq, 8, " to the Apache", 14))
        return(2);

    z_chunkq_remove(cq, 8);
    if (z_chunkq_memcmp(cq, 0, " to the Apache", 14))
        return(3);

    return(0);
}

static int __test_stream (z_test_t *test) {
    struct user_data *data = (struct user_data *)(test->user_data);
    z_stream_t stream;
    char buffer[128];
    uint16_t u16;

    if (z_chunkq_stream(&stream, &(data->chunkq)))
        return(1);

    z_stream_write_uint16(&stream, 512);
    z_stream_write(&stream, "Hello", 5);

    z_stream_seek(&stream, 0);
    z_stream_read_uint16(&stream, &u16);
    if (u16 != 512)
        return(2);

    z_stream_read(&stream, buffer, 5);
    if (z_memcmp(buffer, "Hello", 5))
        return(3);

    return(0);
}

static z_test_t __test_skip_list = {
    .setup      = __test_setup,
    .tear_down  = __test_tear_down,
    .funcs      = {
        __test_crud,
        __test_nblocks,
        __test_tokenize,
        __test_memcmp,
        __test_stream,
        NULL,
    },
};

int main (int argc, char **argv) {
    struct user_data data;
    int res;

    z_memory_init(&(data.memory), z_system_allocator());
    if ((res = z_test_run(&__test_skip_list, &data)))
        printf(" [ !! ] Chunkq %d\n", res);
    else
        printf(" [ ok ] Chunkq\n");

    printf("        - z_chunkq_t            %ubytes\n", (unsigned int)sizeof(z_chunkq_t));
    printf("        - z_chunkq_node_t       %ubytes\n", (unsigned int)sizeof(z_chunkq_node_t));
    printf("        - z_chunkq_extent_t     %ubytes\n", (unsigned int)sizeof(z_chunkq_extent_t));

    return(res);
}
