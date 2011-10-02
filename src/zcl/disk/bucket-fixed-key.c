/*
 *   Copyright 2011 Matteo Bertozzi
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include <zcl/bsearch.h>
#include <zcl/memcpy.h>
#include <zcl/bucket.h>

#define __NODE_HEAD_SIZE            (sizeof(struct node_head))
#define __ITEM_HEAD_SIZE            (sizeof(struct item_head))

#define __ITEM_HEAD(x)              ((struct item_head *)(x))
#define __ITEM_SIZE(bucket)         (__NODE_KLENGTH(bucket) + __ITEM_HEAD_SIZE)

#define __NODE_HEAD(bucket)         ((struct node_head *)((bucket)->data))
#define __NODE_DATA(bucket)         ((bucket)->data + __NODE_HEAD_SIZE)
#define __NODE_KLENGTH(bucket)      (__NODE_HEAD(bucket)->nh_klength)
#define __NODE_ITEMS(bucket)        (__NODE_HEAD(bucket)->nh_items)
#define __NODE_LEVEL(bucket)        (__NODE_HEAD(bucket)->nh_level)
#define __NODE_FREE(bucket)         (__NODE_HEAD(bucket)->nh_free)

#define __bucket_crc(bucket)                                                \
    (bucket)->info->crc_func((bucket->info)->user_data,                     \
                             __NODE_DATA(bucket) + sizeof(uint32_t),        \
                             (bucket)->info->size - sizeof(uint32_t))

struct node_head {
    uint32_t    nh_crc;

    uint16_t    nh_magic;
    uint16_t    nh_level;
    uint32_t    nh_klength;

    uint32_t    nh_items;
    uint32_t    nh_free;
} __attribute__((packed));

struct item_head {
    uint32_t ih_offset;
    uint32_t ih_size;
} __attribute__((packed));

static int __bucket_fixed_key_cmp (void *user_data,
                                   const void *a,
                                   const void *b)
{
    z_bucket_t *bucket = Z_BUCKET(user_data);
    return(bucket->info->key_compare(bucket->info->user_data,
                                     a, __NODE_KLENGTH(bucket),
                                     b, __NODE_KLENGTH(bucket)));
}

/* ==========================================================================
 *  Bucket Plugin Methods
 *  +------+-------+-----+-----+-------+-----+ -+---------+-----+---------+
 *  | Head | Key 0 | I0H | ... | Key N | INH |  | Value N | ... | Value 0 |
 *  +------+-------+-----+-----+-------+-----+--+---------+-----+---------+
 */
static int __bucket_fixed_key_create (z_bucket_t *bucket,
                                      uint16_t magic,
                                      uint16_t level,
                                      uint32_t klength,
                                      uint32_t vlength)
{
    struct node_head *nhead  = __NODE_HEAD(bucket);

    nhead->nh_crc = 0;
    nhead->nh_magic = magic;
    nhead->nh_level = level;
    nhead->nh_klength = klength;
    nhead->nh_items = 0U;
    nhead->nh_free = (bucket->info->size - __NODE_HEAD_SIZE);

    return(0);
}

static int __bucket_fixed_key_open (z_bucket_t *bucket,
                                    uint16_t magic)
{

    struct node_head *nhead = __NODE_HEAD(bucket);

    if (nhead->nh_crc != __bucket_crc(bucket))
        return(1);

    return(0);
}

static int __bucket_fixed_key_close (z_bucket_t *bucket) {
    struct node_head *nhead  = __NODE_HEAD(bucket);

    if (nhead->nh_crc == 0)
        nhead->nh_crc = __bucket_crc(bucket);

    return(0);
}

static int __bucket_fixed_key_search (const z_bucket_t *bucket,
                                      z_bucket_item_t *item,
                                      const void *key,
                                      uint32_t klength)
{
    struct item_head *ihead;
    unsigned int index;
    uint8_t *rkey;

    rkey = z_bsearch_index(key, __NODE_DATA(bucket),
                           __NODE_ITEMS(bucket),
                           __ITEM_SIZE(bucket),
                           __bucket_fixed_key_cmp,
                           (void *)bucket,
                           &index);
    if (rkey == NULL)
        return(-1);

    ihead = __ITEM_HEAD(rkey + __NODE_KLENGTH(bucket));
    item->klength = __NODE_KLENGTH(bucket);
    item->vlength = ihead->ih_size;
    item->key = (const uint8_t *)rkey;
    item->value = __NODE_DATA(bucket) + ihead->ih_offset;

    return(0);
}

static int __bucket_fixed_key_append (z_bucket_t *bucket,
                                      const void *key,
                                      uint32_t klength,
                                      const void *value,
                                      uint32_t vlength)
{
    struct node_head *nhead;
    struct item_head *ihead;
    uint32_t isize;
    uint8_t *ikey;

    nhead = __NODE_HEAD(bucket);
    isize = __ITEM_SIZE(bucket);

    /* TODO: assert(nhead->nh_klength == klength); */

    ikey = __NODE_DATA(bucket) + (nhead->nh_items * isize);
    ihead = __ITEM_HEAD(ikey + nhead->nh_klength);

    /* Insert Item key and head information */
    z_memcpy(ikey, key, nhead->nh_klength);
    ihead->ih_offset = (nhead->nh_free + (nhead->nh_items * isize)) - vlength;
    ihead->ih_size = vlength;

    /* Insert item Value */
    z_memcpy(__NODE_DATA(bucket) + ihead->ih_offset, value, vlength);

    /* Update node stats */
    nhead->nh_free -= (isize + vlength);
    nhead->nh_items++;

    return(0);
}

static uint32_t __bucket_fixed_key_avail (const z_bucket_t *bucket) {
    return(__NODE_FREE(bucket));
}

z_bucket_plug_t z_bucket_fixed_key = {
    .create = __bucket_fixed_key_create,
    .open   = __bucket_fixed_key_open,
    .close  = __bucket_fixed_key_close,

    .search = __bucket_fixed_key_search,
    .append = __bucket_fixed_key_append,

    .avail  = __bucket_fixed_key_avail,
};

