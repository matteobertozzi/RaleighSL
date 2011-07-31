#include <string.h>
#include <stdio.h>

#include <zcl/memory.h>
#include <zcl/test.h>
#include <zcl/tree.h>

struct object {
    const char *key;
    const char *value;
};

struct user_data {
    z_memory_t     memory;
    z_tree_t       tree;
    z_tree_plug_t *plug;
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

static int __object_key_compare (void *user_data,
                                 const void *obj1,
                                 const void *obj2)
{
    return(strcmp(((const struct object *)obj1)->key,
                  ((const struct object *)obj2)->key));
}

static int __test_setup (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;

    z_tree_alloc(&(data->tree), &(data->memory), data->plug,
                 __object_key_compare, __object_free, NULL);

    return(0);
}

static int __test_tear_down (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;
    z_tree_free(&(data->tree));
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

    if (z_tree_insert(&(data->tree), obj))
        return(2);

    if (z_tree_lookup(&(data->tree), &olk) != obj)
        return(3);

    if (z_tree_remove(&(data->tree), &olk))
        return(4);

    if ((obj = __object_alloc(key, value)) == NULL)
        return(1);

    if (z_tree_insert(&(data->tree), obj))
        return(5);

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

static int __test_iter_next (z_tree_iter_t *iter,
                             const void *key)
{
    struct object *obj;

    obj = (struct object *)z_tree_iter_next(iter);
    if (obj == NULL)
        return(key != NULL);

    if (strcmp(obj->key, key))
        return(1);

    return(0);
}

static int __test_iter_prev (z_tree_iter_t *iter,
                             const void *key)
{
    struct object *obj;

    obj = (struct object *)z_tree_iter_prev(iter);
    if (obj == NULL)
        return(key != NULL);

    if (strcmp(obj->key, key))
        return(1);

    return(0);
}

static int __test_iter (z_test_t *test) {
    struct object *obj;
    z_tree_iter_t iter;

    if (__test_crud(test))
        return(1);

    z_tree_iter_init(&iter, &(((struct user_data *)test->user_data)->tree));

    obj = (struct object *)z_tree_iter_lookup_min(&iter);
    if (strcmp(obj->key, "Key 0"))
        return(1);

    if (__test_iter_next(&iter, "Key 1"))
        return(2);

    if (__test_iter_next(&iter, "Key 2"))
        return(3);

    if (__test_iter_next(&iter, "Key 3"))
        return(4);

    if (__test_iter_next(&iter, "Key 4"))
        return(5);

    if (__test_iter_next(&iter, "Key 5"))
        return(6);

    if (__test_iter_next(&iter, NULL))
        return(7);

    obj = (struct object *)z_tree_iter_lookup_max(&iter);
    if (strcmp(obj->key, "Key 5"))
        return(8);

    if (__test_iter_prev(&iter, "Key 4"))
        return(9);

    if (__test_iter_prev(&iter, "Key 3"))
        return(10);

    if (__test_iter_prev(&iter, "Key 2"))
        return(11);

    if (__test_iter_prev(&iter, "Key 1"))
        return(12);

    if (__test_iter_prev(&iter, "Key 0"))
        return(13);

    if (__test_iter_prev(&iter, NULL))
        return(14);

    return(0);
}

static z_test_t __test_tree = {
    .setup      = __test_setup,
    .tear_down  = __test_tear_down,
    .funcs      = {
        __test_crud,
        __test_iter,
        NULL,
    },
};

int main (int argc, char **argv) {
    struct user_data data;
    int res;

    z_memory_init(&(data.memory), z_system_allocator());

    data.plug = &z_tree_avl;
    if ((res = z_test_run(&__test_tree, &data)))
        printf(" [ !! ] Tree (AVL) %d\n", res);
    else
        printf(" [ ok ] Tree (AVL)\n");

    data.plug = &z_tree_red_black;
    if ((res = z_test_run(&__test_tree, &data)))
        printf(" [ !! ] Tree (Red Black)%d\n", res);
    else
        printf(" [ ok ] Tree (Red Black)\n");

    printf("        - z_tree_t              %ubytes\n", (unsigned int)sizeof(z_tree_t));
    printf("        - z_tree_node_t         %ubytes\n", (unsigned int)sizeof(z_tree_node_t));

    return(0);
}
