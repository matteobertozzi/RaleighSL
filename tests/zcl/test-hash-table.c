#include <string.h>
#include <stdio.h>

#include <zcl/hashtable.h>
#include <zcl/memory.h>
#include <zcl/test.h>

struct object {
    const char *key;
    const char *value;
};

struct user_data {
    z_memory_t           memory;
    z_hash_table_t       table;
    z_hash_table_plug_t *plug;
};

static struct object *__object_alloc (const char *key,
                                      const char *value)
{
    struct object *obj;

    if ((obj = (struct object *) malloc(sizeof(struct object))) != NULL) {
        obj->key = key;
        obj->value = value;
    }

    return(obj);
}

static void __object_free (void *user_data, void *obj) {
    free(obj);
}

static unsigned int __object_hash_func (void *user_data,
                                        const void *data)
{
    unsigned int hash = 5381;
    const char *p;

    for (p = ((struct object *)data)->key; *p != '\0'; ++p)
        hash = ((hash << 5) + hash) + *p;

    return(hash);
}

static int __object_key_compare (void *user_data,
                                 const void *obj1,
                                 const void *obj2)
{
    return(strcmp(((const struct object *)obj1)->key,
                  ((const struct object *)obj2)->key));
}

static int __test_setup (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;

    z_hash_table_alloc(&(data->table), &(data->memory), data->plug, 16,
                       __object_hash_func, __object_key_compare,
                      z_hash_table_grow_policy, z_hash_table_shrink_policy,
                       __object_free, NULL);

    return(0);
}

static int __test_tear_down (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;
    z_hash_table_free(&(data->table));
    return(0);
}

static int __test_add (z_test_t *test,
                       const void *key,
                       const void *value)
{
    struct user_data *data = (struct user_data *)test->user_data;
    struct object *obj;
    struct object olk;

    olk.key = key;
    olk.value = NULL;

    if ((obj = __object_alloc(key, value)) == NULL)
        return(1);

    if (z_hash_table_insert(&(data->table), obj))
        return(2);

    if (z_hash_table_lookup(&(data->table), &olk) != obj)
        return(3);

    if (z_hash_table_remove(&(data->table), &olk))
        return(4);

    if ((obj = __object_alloc(key, value)) == NULL)
        return(5);

    if (z_hash_table_insert(&(data->table), obj))
        return(6);

    return(0);
}

static int __test_crud (z_test_t *test) {
    int res;

    if ((res = __test_add(test, "Key 4", "Value 4"))) {
        printf("T1 Failed %d\n", res);
        return(1);
    }

    if ((res = __test_add(test, "Key 2", "Value 2"))) {
        printf("T2 Failed %d\n", res);
        return(2);
    }

    if ((res = __test_add(test, "Key 3", "Value 3"))) {
        printf("T3 Failed %d\n", res);
        return(3);
    }

    if ((res = __test_add(test, "Key 0", "Value 0"))) {
        printf("T4 Failed %d\n", res);
        return(4);
    }

    if ((res = __test_add(test, "Key 5", "Value 5"))) {
        printf("T5 Failed %d\n", res);
        return(5);
    }

    if ((res = __test_add(test, "Key 1", "Value 1"))) {
        printf("T6 Failed %d\n", res);
        return(6);
    }

    if ((res = __test_add(test, "Key 0", NULL))) {
        printf("T7 Failed %d\n", res);
        return(7);
    }

    return(0);
}

static z_test_t __test_tree = {
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

    /* Test Open Addressing Table */
    data.plug = &z_hash_table_open;
    if ((res = z_test_run(&__test_tree, &data)))
        printf(" [ !! ] Hash Table (Open Addressing) %d\n", res);
    else
        printf(" [ ok ] Hash Table (Open Addressing)\n");

    /* Test Chain Table */
    data.plug = &z_hash_table_chain;
    if ((res = z_test_run(&__test_tree, &data)))
        printf(" [ !! ] Hash Table (Chain)%d\n", res);
    else
        printf(" [ ok ] Hash Table (Chain)\n");

    printf("        - z_hash_table_t        %ubytes\n", (unsigned int)sizeof(z_hash_table_t));

    return(0);
}
