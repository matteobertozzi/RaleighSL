#include <string.h>
#include <stdio.h>

#include <zcl/memory.h>
#include <zcl/memcmp.h>
#include <zcl/deque.h>
#include <zcl/test.h>

struct user_data {
    z_memory_t  memory;
    z_deque_t   deque;
};

static int __test_setup (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;

    if (!z_deque_alloc(&(data->deque), &(data->memory), NULL, NULL))
        return(1);

    return(0);
}

static int __test_tear_down (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;
    z_deque_free(&(data->deque));
    return(0);
}

static int __test_crud (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;
    char *object;

    if (z_deque_push(&(data->deque), "OBJECT 1"))
        return(1);

    if (z_deque_push_back(&(data->deque), "OBJECT 2"))
        return(2);

    if (z_deque_push_front(&(data->deque), "OBJECT 0"))
        return(3);

    object = z_deque_peek(&(data->deque));
    if (object == NULL || z_memcmp(object, "OBJECT 0", 8))
        return(4);

    object = z_deque_pop(&(data->deque));
    if (object == NULL || z_memcmp(object, "OBJECT 0", 8))
        return(5);

    if (z_deque_push_back(&(data->deque), "OBJECT 3"))
        return(6);

    object = z_deque_pop_front(&(data->deque));
    if (object == NULL || z_memcmp(object, "OBJECT 1", 8))
        return(7);

    object = z_deque_peek_back(&(data->deque));
    if (object == NULL || z_memcmp(object, "OBJECT 3", 8))
        return(4);

    object = z_deque_pop_back(&(data->deque));
    if (object == NULL || z_memcmp(object, "OBJECT 3", 8))
        return(8);

    object = z_deque_peek_front(&(data->deque));
    if (object == NULL || z_memcmp(object, "OBJECT 2", 8))
        return(4);

    object = z_deque_pop_front(&(data->deque));
    if (object == NULL || z_memcmp(object, "OBJECT 2", 8))
        return(9);

    if (z_deque_peek(&(data->deque)) != NULL)
        return(10);

    object = z_deque_pop(&(data->deque));
    if (object != NULL)
        return(11);

    object = z_deque_pop_front(&(data->deque));
    if (object != NULL)
        return(12);

    object = z_deque_pop_back(&(data->deque));
    if (object != NULL)
        return(13);

    return(0);
}

static z_test_t __test_deque = {
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

    if ((res = z_test_run(&__test_deque, &data)))
        printf(" [ !! ] deque %d\n", res);
    else
        printf(" [ ok ] deque\n");

    printf("        - z_deque_t             %ubytes\n", (unsigned int)sizeof(z_deque_t));
    printf("        - z_deque_node_t        %ubytes\n", (unsigned int)sizeof(z_deque_node_t));

    return(res);
}
