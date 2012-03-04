#ifndef __SSET_PRIVATE_H__
#define __SSET_PRIVATE_H__

#include <raleighfs/errno.h>

#include <zcl/streamslice.h>
#include <zcl/stream.h>
#include <zcl/tree.h>

#define SSET_CONST_OBJECT(x)            Z_CONST_CAST(sset_object_t, x)
#define SSET_OBJECT(x)                  Z_CAST(sset_object_t, x)
#define SSET(x)                         Z_CAST(sset_t, x)

typedef struct sset_object {
    uint64_t cas;
    uint32_t key_size;
    uint32_t data_size;

    uint8_t *data;
    uint8_t  key[1];
} sset_object_t;

typedef struct sset {
    z_tree_t tree;
    int      reversed;
} sset_t;

sset_t *            sset_alloc            (z_memory_t *memory);
void                sset_free             (z_memory_t *memory,
                                           sset_t *sset);

uint64_t            sset_length           (sset_t *sset);

int                 sset_insert           (sset_t *sset,
                                           const z_stream_slice_t *key,
                                           const z_stream_slice_t *value);

void                sset_clear            (sset_t *sset);

sset_object_t *     sset_get              (sset_t *sset,
                                           const z_stream_slice_t *key);
sset_object_t *     sset_get_first        (sset_t *sset);
sset_object_t *     sset_get_last         (sset_t *sset);

void                sset_key_foreach      (sset_t *sset,
                                           const z_stream_slice_t *start,
                                           const z_stream_slice_t *end,
                                           z_foreach_t func,
                                           void *user_data);
void                sset_index_foreach    (sset_t *sset,
                                           uint64_t start,
                                           uint64_t length,
                                           z_foreach_t func,
                                           void *user_data);
void                sset_foreach          (sset_t *sset,
                                           z_foreach_t func,
                                           void *user_data);

int                 sset_remove           (sset_t *sset,
                                           const z_stream_slice_t *key);
int                 sset_remove_range     (sset_t *sset,
                                           const z_stream_slice_t *a,
                                           const z_stream_slice_t *b);
int                 sset_remove_index     (sset_t *sset,
                                           uint64_t start,
                                           uint64_t length);
int                 sset_remove_first     (sset_t *sset);
int                 sset_remove_last      (sset_t *sset);

sset_object_t *     sset_object_alloc     (z_memory_t *memory,
                                           const z_stream_slice_t *key);
void                sset_object_free      (z_memory_t *memory,
                                           sset_object_t *object);
int                 sset_object_set       (sset_object_t *object,
                                           z_memory_t *memory,
                                           const z_stream_slice_t *value);

#endif /* !__SSET_PRIVATE_H__ */

