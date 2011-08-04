#include <string.h>
#include <stdio.h>

#include <zcl/buffer.h>
#include <zcl/test.h>

struct user_data {
    z_memory_t memory;
    z_buffer_t buffer;
};

static unsigned int __grow_policy (void *user_data, unsigned int size) {
    return(size + 16);
}

static int __test_setup (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;

    if (!z_buffer_alloc(&(data->buffer), &(data->memory),
                        __grow_policy, NULL))
    {
        return(1);
    }

    return(0);
}

static int __test_tear_down (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;
    z_buffer_free(&(data->buffer));
    return(0);
}

static int __test_insert (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;

    if (z_buffer_set(&(data->buffer), "Just a test!", 12))
        return(1);

    if (!z_buffer_equal(&(data->buffer), "Just a test!", 12))
        return(2);


    if (z_buffer_append(&(data->buffer), " on this buffer", 15))
        return(3);

    if (!z_buffer_equal(&(data->buffer), "Just a test! on this buffer", 27))
        return(4);


    if (z_buffer_prepend(&(data->buffer), "Hey! ", 5))
        return(5);

    if (!z_buffer_equal(&(data->buffer), "Hey! Just a test! on this buffer", 32))
        return(6);


    if (z_buffer_insert(&(data->buffer), 11, " simple", 7))
        return(7);

    if (!z_buffer_equal(&(data->buffer), "Hey! Just a simple test! on this buffer",39))
        return(8);

    return(0);
}

static int __test_replace (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;

    if (z_buffer_set(&(data->buffer), "This is a simple replace test!", 30))
        return(1);


    if (z_buffer_replace(&(data->buffer), 17, 7, "short", 5))
        return(2);

    if (!z_buffer_equal(&(data->buffer), "This is a simple short test!", 28))
        return(3);

    if (z_buffer_replace(&(data->buffer), 17, 5, "super long", 10))
        return(4);

    if (!z_buffer_equal(&(data->buffer), "This is a simple super long test!", 33))
        return(5);

    return(0);
}

static int __test_remove (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;

    if (z_buffer_set(&(data->buffer), "This is a simple remove test!", 29))
        return(1);


    if (z_buffer_truncate(&(data->buffer), 23))
        return(2);

    if (!z_buffer_equal(&(data->buffer), "This is a simple remove", 23))
        return(3);


    if (z_buffer_remove(&(data->buffer), 0, 5))
        return(4);

    if (!z_buffer_equal(&(data->buffer), "is a simple remove", 18))
        return(5);


    if (z_buffer_remove(&(data->buffer), 11, 7))
        return(6);

    if (!z_buffer_equal(&(data->buffer), "is a simple", 11))
        return(7);


    if (z_buffer_remove(&(data->buffer), 3, 2))
        return(8);

    if (!z_buffer_equal(&(data->buffer), "is simple", 9))
        return(9);

    return(0);
}

static z_test_t __test_skip_list = {
    .setup      = __test_setup,
    .tear_down  = __test_tear_down,
    .funcs      = {
        __test_insert,
        __test_replace,
        __test_remove,
        NULL,
    },
};

int main (int argc, char **argv) {
    struct user_data data;
    int res;

    z_memory_init(&(data.memory), z_system_allocator());
    if ((res = z_test_run(&__test_skip_list, &data)))
        printf(" [ !! ] Buffer %d\n", res);
    else
        printf(" [ ok ] Buffer\n");

    printf("        - z_buffer_t            %ubytes\n", (unsigned int)sizeof(z_buffer_t));

    return(res);
}
