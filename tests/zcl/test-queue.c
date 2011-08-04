#include <string.h>
#include <stdio.h>

#include <zcl/memory.h>
#include <zcl/memcmp.h>
#include <zcl/queue.h>
#include <zcl/test.h>

struct user_data {
    z_memory_t  memory;
    z_queue_t   queue;
};

static int __test_setup (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;

    if (!z_queue_alloc(&(data->queue), &(data->memory), NULL, NULL))
        return(1);

    return(0);
}

static int __test_tear_down (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;
    z_queue_free(&(data->queue));
    return(0);
}

static int __test_crud (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;
    char *object;

    if (z_queue_push(&(data->queue), "OBJECT 1"))
        return(1);

    if (z_queue_push(&(data->queue), "OBJECT 2"))
        return(2);

    object = z_queue_peek(&(data->queue));
    if (object == NULL || z_memcmp(object, "OBJECT 1", 8))
        return(3);

    object = z_queue_pop(&(data->queue));
    if (object == NULL || z_memcmp(object, "OBJECT 1", 8))
        return(4);

    if (z_queue_push(&(data->queue), "OBJECT 3"))
        return(5);

    object = z_queue_pop(&(data->queue));
    if (object == NULL || z_memcmp(object, "OBJECT 2", 8))
        return(6);

    object = z_queue_pop(&(data->queue));
    if (object == NULL || z_memcmp(object, "OBJECT 3", 8))
        return(7);

    if (z_queue_peek(&(data->queue)) != NULL)
        return(8);

    object = z_queue_pop(&(data->queue));
    if (object != NULL)
        return(9);

    return(0);
}

static z_test_t __test_queue = {
    .setup      = __test_setup,
    .tear_down  = __test_tear_down,
    .funcs      = {
        __test_crud,
        NULL,
    },
};

int main (int argc, char **argv) {
    struct user_data data;
    int res;

    z_memory_init(&(data.memory), z_system_allocator());

    if ((res = z_test_run(&__test_queue, &data)))
        printf(" [ !! ] Queue %d\n", res);
    else
        printf(" [ ok ] Queue\n");

    printf("        - z_queue_t             %ubytes\n", (unsigned int)sizeof(z_queue_t));
    printf("        - z_queue_node_t        %ubytes\n", (unsigned int)sizeof(z_queue_node_t));

    return(res);
}
