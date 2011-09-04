#include <string.h>
#include <stdio.h>

#include <zcl/memory.h>
#include <zcl/memcmp.h>
#include <zcl/cache.h>
#include <zcl/hash.h>
#include <zcl/test.h>

#define __CACHE_SIZE            8

struct user_data {
    z_memory_t  memory;
    z_cache_t   cache;
};

struct object {
    unsigned int key;
};

static struct object *__object_alloc (unsigned int key) {
    struct object *obj;

    if ((obj = (struct object *) malloc(sizeof(struct object))) != NULL) {
        obj->key = key;
    }

    return(obj);
}

static void __object_free (void *user_data, void *obj) {
    free(obj);
}

static unsigned int __hash_func (void *data, const void *obj) {
    return(((struct object *)obj)->key);
}

static int __cmp_func (void *data, const void *a, const void *b) {
    return(((struct object *)a)->key - ((struct object *)b)->key);
}

static int __test_setup (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;

    if (!z_cache_alloc(&(data->cache), &(data->memory),
                       __CACHE_SIZE, __hash_func, __cmp_func,
                       __object_free, data))
    {
        return(1);
    }

    return(0);
}


static int __test_tear_down (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;
    z_cache_free(&(data->cache));
    return(0);
}

static int __test_crud (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;
    struct object *obj;
    unsigned int i;

    for (i = 1; i <= (__CACHE_SIZE << 1); ++i) {
        obj =  __object_alloc(i);
        z_cache_add(&(data->cache), obj);
        if (z_cache_lookup(&(data->cache), obj) != obj)
            return(1);
    }

    for (i = 1; i <= __CACHE_SIZE; ++i) {
        if (z_cache_lookup(&(data->cache), &i) != NULL)
            return(2);
    }

    for (i = __CACHE_SIZE + 1; i <= (__CACHE_SIZE << 1); ++i) {
        if (z_cache_lookup(&(data->cache), obj) == NULL)
            return(3);
    }

    return(0);
}

static z_test_t __test_cache = {
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

    if ((res = z_test_run(&__test_cache, &data)))
        printf(" [ !! ] Cache %d\n", res);
    else
        printf(" [ ok ] Cache\n");

    printf("        - z_cache_t             %ubytes\n", (unsigned int)sizeof(z_cache_t));
    printf("        - z_cache_node_t        %ubytes\n", (unsigned int)sizeof(z_cache_node_t));

    return(res);
}
