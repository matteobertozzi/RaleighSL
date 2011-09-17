#include <zcl/snprintf.h>
#include <zcl/extents.h>
#include <zcl/strcmp.h>
#include <zcl/test.h>

#include <stdio.h>

struct extent {
    Z_EXTENT_TYPE
    char item;
};

struct user_data {
    z_memory_t      memory;
    z_extent_tree_t tree;
    char item;
};

static z_extent_t *__extent_alloc (struct user_data *data,
                                   uint64_t offset, uint64_t length)
{
    z_extent_t *extent;

    if ((extent = (z_extent_t *) malloc(sizeof(struct extent))) != NULL) {
        Z_EXTENT(extent)->offset = offset;
        Z_EXTENT(extent)->length = length;
        ((struct extent *)extent)->item = data->item++;
    }

    return(extent);
}

static void __extent_free (void *data, void *ptr) {
    free(ptr);
}

static int __extent_head_trim (void *user_data,
                               z_extent_t *extent,
                               uint64_t trim)
{
    return(0);
}

static int __extent_tail_trim (void *user_data,
                               z_extent_t *extent,
                               uint64_t trim)
{
    return(0);
}

static int __extent_split (void *user_data,
                           const z_extent_t *extent,
                           uint64_t offset,
                           z_extent_t **left,
                           z_extent_t **right)
{
    *right = __extent_alloc(user_data, offset,
                            extent->offset + extent->length - offset);

    *left = (z_extent_t *)extent;
    (*left)->length = offset - extent->offset;

    return(0);
}

static z_extent_plug_t __test_extent_plug = {
    .head_trim = __extent_head_trim,
    .tail_trim = __extent_tail_trim,
    .split     = __extent_split,
};

static int __test_setup (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;

    data->item = 'A';
    if (!z_extent_tree_alloc(&(data->tree), &(data->memory),
                             &__test_extent_plug,
                             __extent_free,
                             data))
    {
        return(1);
    }

    return(0);
}

static int __test_tear_down (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;
    z_extent_tree_free(&(data->tree));
    return(0);
}

static int __tree_debug_iter (z_extent_tree_t *tree, const char *res) {
    z_tree_iter_t iter;
    char buffer[256];
    unsigned int n;
    void *e;

    n = 0;
    z_tree_iter_init(&iter, Z_TREE(tree));
    e = z_tree_iter_lookup_min(&iter);
    while (e != NULL) {
        n += z_snprintf(buffer + n, sizeof(buffer) - n, "[%u8d:%u8d(%c)]->",
                        Z_EXTENT(e)->offset, Z_EXTENT(e)->length,
                        ((struct extent *)e)->item);

        e = z_tree_iter_next(&iter);
    }
    n += z_snprintf(buffer + n, sizeof(buffer) - n, "X");
    return(z_strcmp(res, buffer));
}

static int __test_crud (z_test_t *test) {
    struct user_data *data = (struct user_data *)test->user_data;

    z_extent_tree_insert(&(data->tree), __extent_alloc(data, 0, 10));
    if (__tree_debug_iter(&(data->tree), "[0:10(A)]->X"))
        return(1);

    z_extent_tree_insert(&(data->tree), __extent_alloc(data, 0, 10));
    if (__tree_debug_iter(&(data->tree), "[0:10(B)]->[10:10(A)]->X"))
        return(2);

    z_extent_tree_insert(&(data->tree), __extent_alloc(data, 10, 5));
    if (__tree_debug_iter(&(data->tree), "[0:10(B)]->[10:5(C)]->[15:10(A)]->X"))
        return(3);

    z_extent_tree_insert(&(data->tree), __extent_alloc(data, 15, 5));
    if (__tree_debug_iter(&(data->tree), "[0:10(B)]->[10:5(C)]->[15:5(D)]->[20:10(A)]->X"))
        return(4);

    z_extent_tree_insert(&(data->tree), __extent_alloc(data, 5, 10));
    if (__tree_debug_iter(&(data->tree), "[0:5(B)]->[5:10(E)]->[15:5(F)]->[20:5(C)]->[25:5(D)]->[30:10(A)]->X"))
        return(5);

    z_extent_tree_insert(&(data->tree), __extent_alloc(data, 15, 20));
    if (__tree_debug_iter(&(data->tree), "[0:5(B)]->[5:10(E)]->[15:20(G)]->[35:5(F)]->[40:5(C)]->[45:5(D)]->[50:10(A)]->X"))
        return(6);

    z_extent_tree_insert(&(data->tree), __extent_alloc(data, 45, 10));
    if (__tree_debug_iter(&(data->tree), "[0:5(B)]->[5:10(E)]->[15:20(G)]->[35:5(F)]->[40:5(C)]->[45:10(H)]->[55:5(D)]->[60:10(A)]->X"))
        return(7);

    z_extent_tree_insert(&(data->tree), __extent_alloc(data, 70, 10));
    if (__tree_debug_iter(&(data->tree), "[0:5(B)]->[5:10(E)]->[15:20(G)]->[35:5(F)]->[40:5(C)]->[45:10(H)]->[55:5(D)]->[60:10(A)]->[70:10(I)]->X"))
        return(8);

    z_extent_tree_insert(&(data->tree), __extent_alloc(data, 70, 15));
    if (__tree_debug_iter(&(data->tree), "[0:5(B)]->[5:10(E)]->[15:20(G)]->[35:5(F)]->[40:5(C)]->[45:10(H)]->[55:5(D)]->[60:10(A)]->[70:15(J)]->[85:10(I)]->X"))
        return(9);

    z_extent_tree_remove(&(data->tree), 0, 5); /* S1 */
    if (__tree_debug_iter(&(data->tree), "[0:10(E)]->[10:20(G)]->[30:5(F)]->[35:5(C)]->[40:10(H)]->[50:5(D)]->[55:10(A)]->[65:15(J)]->[80:10(I)]->X"))
        return(10);

    z_extent_tree_remove(&(data->tree), 0, 5); /* S2 */
    if (__tree_debug_iter(&(data->tree), "[0:5(E)]->[5:20(G)]->[25:5(F)]->[30:5(C)]->[35:10(H)]->[45:5(D)]->[50:10(A)]->[60:15(J)]->[75:10(I)]->X"))
        return(11);

    z_extent_tree_remove(&(data->tree), 1, 4); /* S3 */
    if (__tree_debug_iter(&(data->tree), "[0:1(E)]->[1:20(G)]->[21:5(F)]->[26:5(C)]->[31:10(H)]->[41:5(D)]->[46:10(A)]->[56:15(J)]->[71:10(I)]->X"))
        return(12);

    z_extent_tree_remove(&(data->tree), 0, 1); /* S1 */
    if (__tree_debug_iter(&(data->tree), "[0:20(G)]->[20:5(F)]->[25:5(C)]->[30:10(H)]->[40:5(D)]->[45:10(A)]->[55:15(J)]->[70:10(I)]->X"))
        return(13);

    z_extent_tree_remove(&(data->tree), 6, 5); /* S4 */
    if (__tree_debug_iter(&(data->tree), "[0:5(G)]->[5:10(K)]->[15:5(F)]->[20:5(C)]->[25:10(H)]->[35:5(D)]->[40:10(A)]->[50:15(J)]->[65:10(I)]->X"))
        return(14);

    z_extent_tree_remove(&(data->tree), 5, 30); /* S5 */
    if (__tree_debug_iter(&(data->tree), "[0:5(G)]->[5:5(D)]->[10:10(A)]->[20:15(J)]->[35:10(I)]->X"))
        return(15);

    z_extent_tree_remove(&(data->tree), 5, 10); /* S6 */
    if (__tree_debug_iter(&(data->tree), "[0:5(G)]->[5:5(A)]->[10:15(J)]->[25:10(I)]->X"))
        return(16);

    z_extent_tree_remove(&(data->tree), 15, 20); /* S7 */
    if (__tree_debug_iter(&(data->tree), "[0:5(G)]->[5:5(A)]->[10:5(J)]->X"))
        return(17);

    z_extent_tree_remove(&(data->tree), 2, 12); /* S8 */
    if (__tree_debug_iter(&(data->tree), "[0:2(G)]->[2:1(J)]->X"))
        return(18);

    return(0);
}

static z_test_t __test_extent_tree = {
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

    if ((res = z_test_run(&__test_extent_tree, &data)))
        printf(" [ !! ] Extent Tree %d\n", res);
    else
        printf(" [ ok ] Extent Tree\n");

    printf("        - z_extent_t            %ubytes\n", (unsigned int)sizeof(z_extent_t));
    printf("        - z_extent_tree_t       %ubytes\n", (unsigned int)sizeof(z_extent_tree_t));

    return(res);
}

