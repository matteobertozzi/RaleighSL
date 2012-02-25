#include <zcl/memcmp.h>

#include "sset_p.h"

static int __key_stream_compare (void *data, const void *a, const void *b) {
    const sset_object_t *sa = SSET_CONST_OBJECT(a);
    z_stream_extent_t *sb = (z_stream_extent_t *)b;
    unsigned int min_len;
    int cmp;

    min_len = z_min(sa->key_size, sb->length);
    if ((cmp = z_stream_memcmp(sb->stream, sb->offset, sa->key, min_len)))
        return(-cmp);

    return(sa->key_size - sb->length);
}

static int __key_compare (void *user_data, const void *a, const void *b) {
    const sset_object_t *sa = SSET_CONST_OBJECT(a);
    const sset_object_t *sb = SSET_CONST_OBJECT(b);
    return(z_memncmp(sa->key, sa->key_size, sb->key, sb->key_size));
}

static void __free_data (void *user_data, void *ptr) {
    sset_object_free(Z_MEMORY(user_data), SSET_OBJECT(ptr));
}

sset_t *sset_alloc (z_memory_t *memory) {
    sset_t *sset;

    if ((sset = z_memory_struct_alloc(memory, sset_t)) == NULL)
        return(NULL);

    if (!z_tree_alloc(&(sset->tree), memory, &z_tree_red_black,
                      __key_compare, __free_data, memory))
    {
        z_memory_struct_free(memory, sset_t, sset);
        return(NULL);
    }

    return(sset);
}

void sset_free (z_memory_t *memory, sset_t *sset) {
    z_tree_free(&(sset->tree));
    z_memory_struct_free(memory, sset_t, sset);
}

void sset_clear (sset_t *sset) {
    z_tree_clear(&(sset->tree));
}

uint64_t sset_length (sset_t *sset) {
    return(sset->tree.size);
}

int sset_insert (sset_t *sset,
                 const z_stream_extent_t *key,
                 const z_stream_extent_t *value)
{
    sset_object_t *object;
    z_memory_t *memory;

    memory = z_object_memory(&(sset->tree));
    if ((object = sset_object_alloc(memory, key)) == NULL)
        return(1);

    if (sset_object_set(object, memory, value)) {
        sset_object_free(memory, object);
        return(2);
    }

    if (z_tree_insert(&(sset->tree), object)) {
        sset_object_free(memory, object);
        return(3);
    }

    return(0);
}

sset_object_t *sset_get (sset_t *sset, const z_stream_extent_t *key) {
    return(SSET_OBJECT(z_tree_lookup_custom(&(sset->tree),
                                            __key_stream_compare, key)));
}

void sset_index_foreach (sset_t *sset,
                         uint64_t start,
                         uint64_t length,
                         z_foreach_t func,
                         void *user_data)
{
    sset_object_t *object;
    z_tree_iter_t iter;
    uint64_t i;

    z_tree_iter_init(&iter, &(sset->tree));

    /* Loop until i == start */
    object = SSET_OBJECT(z_tree_iter_lookup_min(&iter));
    for (i = 0; i < start && object != NULL; ++i)
        object = SSET_OBJECT(z_tree_iter_next(&iter));

    /* Loop for length items */
    for (i = 0; i < length && object != NULL; ++i) {
        func(user_data, object);

        object = SSET_OBJECT(z_tree_iter_next(&iter));
    }
}

void sset_key_foreach (sset_t *sset,
                       const z_stream_extent_t *start,
                       const z_stream_extent_t *end,
                       z_foreach_t func,
                       void *user_data)
{
    sset_object_t *object;
    z_tree_iter_t iter;

    z_tree_iter_init(&iter, &(sset->tree));

    object = SSET_OBJECT(z_tree_iter_lookup_min(&iter));
    while (object != NULL) {
        if (__key_stream_compare(NULL, object, start) >= 0)
            break;

        object = SSET_OBJECT(z_tree_iter_next(&iter));
    }

    while (object != NULL) {
        if (__key_stream_compare(NULL, object, end) > 0)
            break;

        func(user_data, object);

        object = SSET_OBJECT(z_tree_iter_next(&iter));
    }
}

void sset_foreach (sset_t *sset, z_foreach_t func, void *user_data) {
    z_tree_iter_t iter;
    void *object;

    z_tree_iter_init(&iter, &(sset->tree));
    object = z_tree_iter_lookup_min(&iter);
    while (object != NULL) {
        func(user_data, object);
        object = z_tree_iter_next(&iter);
    }
}

sset_object_t *sset_get_first (sset_t *sset) {
    return(SSET_OBJECT(z_tree_lookup_min(&(sset->tree))));
}

sset_object_t *sset_get_last (sset_t *sset) {
    return(SSET_OBJECT(z_tree_lookup_max(&(sset->tree))));
}

int sset_remove (sset_t *sset, const z_stream_extent_t *key) {
    void *p;

    if (!(p = z_tree_lookup_custom(&(sset->tree), __key_stream_compare, key)))
        return(1);

    return(z_tree_remove(&(sset->tree), p));
}

int sset_remove_range (sset_t *sset,
                       const z_stream_extent_t *a,
                       const z_stream_extent_t *b)
{
    void *ka;
    void *kb;

    ka = z_tree_lookup_ceil_custom(&(sset->tree), __key_stream_compare, a);
    kb = z_tree_lookup_floor_custom(&(sset->tree), __key_stream_compare, b);
    if (ka == NULL || kb == NULL)
        return(1);

    z_tree_remove_range(&(sset->tree), ka, kb);
    z_tree_remove(&(sset->tree), ka);
    z_tree_remove(&(sset->tree), kb);

    return(0);
}

int sset_remove_index (sset_t *sset, uint64_t start, uint64_t length) {
    return(z_tree_remove_index(&(sset->tree), start, length));
}

int sset_remove_first (sset_t *sset) {
    return(z_tree_remove_min(&(sset->tree)));
}

int sset_remove_last (sset_t *sset) {
    return(z_tree_remove_max(&(sset->tree)));
}


sset_object_t *sset_object_alloc (z_memory_t *memory,
                                  const z_stream_extent_t *key)
{
    sset_object_t *obj;
    unsigned int size;

    size = sizeof(sset_object_t) + (key->length - 1);
    if ((obj = z_memory_block_alloc(memory, sset_object_t, size)) != NULL) {
        obj->cas = 0U;
        obj->data = NULL;
        obj->data_size = 0U;
        obj->key_size = key->length;
        z_stream_read_extent(key, obj->key);
    }

    return(obj);
}

void sset_object_free (z_memory_t *memory,
                       sset_object_t *object)
{
    if (object->data != NULL)
        z_memory_blob_free(memory, object->data);
    z_memory_block_free(memory, object);
}

int sset_object_set (sset_object_t *object,
                     z_memory_t *memory,
                     const z_stream_extent_t *value)
{
    void *data;

    if ((data = z_memory_blob_alloc(memory, value->length)) == NULL)
        return(1);

    if (object->data != NULL)
        z_memory_blob_free(memory, object->data);

    z_stream_read_extent(value, data);
    object->data = data;
    object->data_size = value->length;
    object->cas++;

    return(0);
}

