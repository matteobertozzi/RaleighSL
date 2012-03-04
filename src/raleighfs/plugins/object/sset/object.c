#include "sset_p.h"
#include "sset.h"

typedef void (*sset_range_t) (raleighfs_t *fs,
                              raleighfs_object_t *object,
                              const z_stream_slice_t *a,
                              const z_stream_slice_t *b,
                              z_stream_t *stream);

typedef void (*sset_index_t) (raleighfs_t *fs,
                              raleighfs_object_t *object,
                              uint64_t start,
                              uint64_t length,
                              z_stream_t *stream);

typedef void (*sset_item_t) (raleighfs_t *fs,
                             raleighfs_object_t *object,
                             const z_stream_slice_t *item,
                             z_stream_t *stream);

#define __SSET(object)              ((sset_t *)((object)->internal->membufs))

static raleighfs_errno_t __object_create (raleighfs_t *fs,
                                          raleighfs_object_t *object)
{
    sset_t *sset;

    if ((sset = sset_alloc(z_object_memory(fs))) == NULL)
        return(RALEIGHFS_ERRNO_NO_MEMORY);

    object->internal->membufs = sset;

    return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __object_open (raleighfs_t *fs,
                                        raleighfs_object_t *object)
{
    return(RALEIGHFS_ERRNO_NOT_IMPLEMENTED);
}

static raleighfs_errno_t __object_close (raleighfs_t *fs,
                                         raleighfs_object_t *object)
{
    sset_free(z_object_memory(fs), __SSET(object));
    return(RALEIGHFS_ERRNO_NONE);
}

static raleighfs_errno_t __object_sync (raleighfs_t *fs,
                                        raleighfs_object_t *object)
{
    return(RALEIGHFS_ERRNO_NOT_IMPLEMENTED);
}

static raleighfs_errno_t __object_unlink (raleighfs_t *fs,
                                          raleighfs_object_t *object)
{
    sset_free(z_object_memory(fs), __SSET(object));
    return(RALEIGHFS_ERRNO_NONE);
}

static void __serialize_sset_object (z_stream_t *stream,
                                     sset_object_t *sobject)
{
    if (sobject != NULL) {
        z_stream_write_uint32(stream, sobject->key_size);
        z_stream_write_uint32(stream, sobject->data_size);
        z_stream_write(stream, sobject->key, sobject->key_size);
        z_stream_write(stream, sobject->data, sobject->data_size);
    } else {
        z_stream_write_uint32(stream, 0);
    }
}

static void __sset_get_range (raleighfs_t *fs,
                              raleighfs_object_t *object,
                              const z_stream_slice_t *a,
                              const z_stream_slice_t *b,
                              z_stream_t *stream)
{
    sset_key_foreach(__SSET(object), a, b,
                     (z_foreach_t)__serialize_sset_object, stream);
}

static void __sset_get_index (raleighfs_t *fs,
                              raleighfs_object_t *object,
                              uint64_t start,
                              uint64_t length,
                              z_stream_t *stream)
{
    sset_index_foreach(__SSET(object), start, length,
                       (z_foreach_t)__serialize_sset_object, stream);
}

static void __sset_get (raleighfs_t *fs,
                        raleighfs_object_t *object,
                        const z_stream_slice_t *key,
                        z_stream_t *stream)
{
    sset_object_t *sobject;

    sobject = sset_get(__SSET(object), key);
    __serialize_sset_object(stream, sobject);
}

static void __sset_rm_range (raleighfs_t *fs,
                             raleighfs_object_t *object,
                             const z_stream_slice_t *a,
                             const z_stream_slice_t *b,
                             z_stream_t *stream)
{
    sset_remove_range(__SSET(object), a, b);
}

static void __sset_rm_index (raleighfs_t *fs,
                             raleighfs_object_t *object,
                             uint64_t start,
                             uint64_t length,
                             z_stream_t *stream)
{
    sset_remove_index(__SSET(object), start, length);
}

static void __sset_rm (raleighfs_t *fs,
                       raleighfs_object_t *object,
                       const z_stream_slice_t *key,
                       z_stream_t *stream)
{
    sset_remove(__SSET(object), key);
}

static int __sset_write_keys (void *user_data, void *object) {
    sset_object_t *sobject = SSET_OBJECT(object);
    z_stream_t *stream = Z_STREAM(user_data);
    z_stream_write_uint32(stream, sobject->key_size);
    z_stream_write(stream, sobject->key, sobject->key_size);
    return(0);
}

static void __object_range (raleighfs_t *fs,
                            raleighfs_object_t *object,
                            z_message_t *msg,
                            z_stream_t *response,
                            sset_range_t range_func)
{
    z_message_stream_t stream;
    z_stream_slice_t a, b;
    uint32_t alen, blen;

    z_message_request_stream(msg, &stream);
    z_stream_read_uint32(Z_STREAM(&stream), &alen);
    z_stream_read_uint32(Z_STREAM(&stream), &blen);
    z_stream_slice(&a, Z_STREAM(&stream), 8, alen);
    z_stream_slice(&b, Z_STREAM(&stream), 8 + alen, blen);

    range_func(fs, object, &a, &b, response);
}

static void __object_index (raleighfs_t *fs,
                            raleighfs_object_t *object,
                            z_message_t *msg,
                            z_stream_t *response,
                            sset_index_t index_func)
{
    z_message_stream_t stream;
    uint64_t length;
    uint64_t start;

    z_message_request_stream(msg, &stream);
    z_stream_read_uint64(Z_STREAM(&stream), &start);
    z_stream_read_uint64(Z_STREAM(&stream), &length);

    index_func(fs, object, start, length, response);
}

static void __object_items (raleighfs_t *fs,
                            raleighfs_object_t *object,
                            z_message_t *msg,
                            z_stream_t *response,
                            sset_item_t item_func)
{
    z_message_stream_t stream;
    z_stream_slice_t key;
    uint32_t length;
    uint32_t count;

    z_message_request_stream(msg, &stream);
    z_stream_read_uint32(Z_STREAM(&stream), &count);

    while (count--) {
        z_stream_read_uint32(Z_STREAM(&stream), &length);
        z_stream_slice(&key, Z_STREAM(&stream), stream.offset, length);
        z_stream_seek(Z_STREAM(&stream), key.offset + key.length);
        item_func(fs, object, &key, response);
    }
}


static raleighfs_errno_t __object_query (raleighfs_t *fs,
                                         raleighfs_object_t *object,
                                         z_message_t *msg)
{
    raleighfs_errno_t errno = RALEIGHFS_ERRNO_NONE;
    z_message_stream_t stream;
    sset_object_t *sobject;

    z_message_response_stream(msg, &stream);

    switch (z_message_type(msg)) {
        case RALEIGHFS_SSET_GET:
            __object_items(fs, object, msg, Z_STREAM(&stream), __sset_get);
            z_stream_write_uint32(Z_STREAM(&stream), 0);
            break;
        case RALEIGHFS_SSET_GET_RANGE:
            __object_range(fs, object, msg, Z_STREAM(&stream), __sset_get_range);
            z_stream_write_uint32(Z_STREAM(&stream), 0);
            break;
        case RALEIGHFS_SSET_GET_INDEX:
            __object_index(fs, object, msg, Z_STREAM(&stream), __sset_get_index);
            z_stream_write_uint32(Z_STREAM(&stream), 0);
            break;
        case RALEIGHFS_SSET_GET_FIRST:
            sobject = sset_get_first(__SSET(object));
            __serialize_sset_object(Z_STREAM(&stream), sobject);
            z_stream_write_uint32(Z_STREAM(&stream), 0);
            break;
        case RALEIGHFS_SSET_GET_LAST:
            sobject = sset_get_last(__SSET(object));
            __serialize_sset_object(Z_STREAM(&stream), sobject);
            z_stream_write_uint32(Z_STREAM(&stream), 0);
            break;
        case RALEIGHFS_SSET_GET_KEYS:
            z_stream_write_uint64(Z_STREAM(&stream), sset_length(__SSET(object)));
            sset_foreach(__SSET(object), __sset_write_keys, &stream);
            break;
        case RALEIGHFS_SSET_LENGTH:
            z_stream_write_uint64(Z_STREAM(&stream), sset_length(__SSET(object)));
            break;
        case RALEIGHFS_SSET_STATS:
        default:
            errno = RALEIGHFS_ERRNO_NOT_IMPLEMENTED;
            break;
    }

    z_message_set_state(msg, errno);
    return(errno);
}

static raleighfs_errno_t __object_insert (raleighfs_t *fs,
                                          raleighfs_object_t *object,
                                          z_message_t *msg)
{
    z_message_stream_t stream;
    raleighfs_errno_t errno;
    z_stream_slice_t data;
    z_stream_slice_t key;
    uint32_t klength;
    uint32_t vlength;

    z_message_request_stream(msg, &stream);
    z_stream_read_uint32(Z_STREAM(&stream), &klength);
    z_stream_read_uint32(Z_STREAM(&stream), &vlength);
    z_stream_slice(&key, Z_STREAM(&stream), 8, klength);
    z_stream_slice(&data, Z_STREAM(&stream), 8 + klength, vlength);

    switch (z_message_type(msg)) {
        case RALEIGHFS_SSET_INSERT:
            if (sset_insert(__SSET(object), &key, &data))
                errno = RALEIGHFS_ERRNO_NO_MEMORY;
            else
                errno = RALEIGHFS_ERRNO_NONE;
            break;
        default:
            errno = RALEIGHFS_ERRNO_NOT_IMPLEMENTED;
            break;
    }

    z_message_set_state(msg, errno);
    return(errno);
}

static raleighfs_errno_t __object_update (raleighfs_t *fs,
                                          raleighfs_object_t *object,
                                          z_message_t *msg)
{
    z_message_stream_t stream;
    raleighfs_errno_t errno;
    z_stream_slice_t data;
    z_stream_slice_t key;
    uint32_t klength;
    uint32_t vlength;

    z_message_request_stream(msg, &stream);
    z_stream_read_uint32(Z_STREAM(&stream), &klength);
    z_stream_read_uint32(Z_STREAM(&stream), &vlength);
    z_stream_slice(&key, Z_STREAM(&stream), 8, klength);
    z_stream_slice(&data, Z_STREAM(&stream), 8 + klength, vlength);

    switch (z_message_type(msg)) {
        case RALEIGHFS_SSET_UPDATE:
            if (sset_insert(__SSET(object), &key, &data))
                errno = RALEIGHFS_ERRNO_NO_MEMORY;
            else
                errno = RALEIGHFS_ERRNO_NONE;
            break;
        default:
            errno = RALEIGHFS_ERRNO_NOT_IMPLEMENTED;
            break;
    }

    z_message_set_state(msg, errno);
    return(errno);
}
#include <stdio.h>
static raleighfs_errno_t __object_remove (raleighfs_t *fs,
                                          raleighfs_object_t *object,
                                          z_message_t *msg)
{
    raleighfs_errno_t errno = RALEIGHFS_ERRNO_NONE;
    z_message_stream_t stream;
    sset_object_t *sobject;

    z_message_response_stream(msg, &stream);

    switch (z_message_type(msg)) {
        case RALEIGHFS_SSET_POP:
            __object_items(fs, object, msg, Z_STREAM(&stream), __sset_get);
            __object_items(fs, object, msg, Z_STREAM(&stream), __sset_rm);
            break;
        case RALEIGHFS_SSET_POP_RANGE:
            __object_range(fs, object, msg, Z_STREAM(&stream), __sset_get_range);
            __object_range(fs, object, msg, Z_STREAM(&stream), __sset_rm_range);
            break;
        case RALEIGHFS_SSET_POP_INDEX:
            __object_index(fs, object, msg, Z_STREAM(&stream), __sset_get_index);
            __object_index(fs, object, msg, Z_STREAM(&stream), __sset_rm_index);
            break;
        case RALEIGHFS_SSET_POP_FIRST:
            sobject = sset_get_first(__SSET(object));
            __serialize_sset_object(Z_STREAM(&stream), sobject);
            z_stream_write_uint32(Z_STREAM(&stream), 0);
            sset_remove_first(__SSET(object));
            break;
        case RALEIGHFS_SSET_POP_LAST:
            sobject = sset_get_last(__SSET(object));
            __serialize_sset_object(Z_STREAM(&stream), sobject);
            z_stream_write_uint32(Z_STREAM(&stream), 0);
            sset_remove_last(__SSET(object));
            break;
        case RALEIGHFS_SSET_RM:
            __object_items(fs, object, msg, Z_STREAM(&stream), __sset_rm);
            break;
        case RALEIGHFS_SSET_RM_RANGE:
            __object_range(fs, object, msg, Z_STREAM(&stream), __sset_rm_range);
            break;
        case RALEIGHFS_SSET_RM_INDEX:
            __object_index(fs, object, msg, Z_STREAM(&stream), __sset_rm_index);
            break;
        case RALEIGHFS_SSET_RM_FIRST:
            sset_remove_first(__SSET(object));
            break;
        case RALEIGHFS_SSET_RM_LAST:
            sset_remove_last(__SSET(object));
            break;
        case RALEIGHFS_SSET_CLEAR:
            sset_clear(__SSET(object));
            break;
        default:
            errno = RALEIGHFS_ERRNO_NOT_IMPLEMENTED;
            break;
    }

    z_message_set_state(msg, errno);
    return(errno);
}

static raleighfs_errno_t __object_ioctl (raleighfs_t *fs,
                                         raleighfs_object_t *object,
                                         z_message_t *msg)
{
    raleighfs_errno_t errno;

    switch (z_message_type(msg)) {
        case RALEIGHFS_SSET_REVERSE:
        default:
            errno = RALEIGHFS_ERRNO_NOT_IMPLEMENTED;
            break;
    }

    z_message_set_state(msg, errno);
    return(errno);
}

const uint8_t RALEIGHFS_OBJECT_SSET_UUID[16] = {
    132,238,186,222,246,41,75,245,132,74,231,238,27,227,0,125
};

raleighfs_object_plug_t raleighfs_object_sset = {
    .info = {
        .type = RALEIGHFS_PLUG_TYPE_OBJECT,
        .uuid = RALEIGHFS_OBJECT_SSET_UUID,
        .description = "Sorted Set Object",
        .label = "object-sset",
    },

    .create     = __object_create,
    .open       = __object_open,
    .close      = __object_close,
    .sync       = __object_sync,
    .unlink     = __object_unlink,

    .query      = __object_query,
    .insert     = __object_insert,
    .update     = __object_update,
    .remove     = __object_remove,
    .ioctl      = __object_ioctl,
};
