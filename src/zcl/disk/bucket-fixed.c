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

#define __NODE_HEAD(bucket)         ((struct node_head *)((bucket)->data))
#define __NODE_DATA(bucket)         ((bucket)->data + __NODE_HEAD_SIZE)
#define __NODE_END_DATA(bucket)     ((bucket)->data + (bucket)->info->size)
#define __NODE_KLENGTH(bucket)      (__NODE_HEAD(bucket)->nh_klength)
#define __NODE_VLENGTH(bucket)      (__NODE_HEAD(bucket)->nh_vlength)
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
    uint32_t    nh_vlength;

    uint32_t    nh_items;
    uint32_t    nh_free;
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
 *  +------+-------+-----+-------+--+---------+-----+---------+
 *  | Head | Key 0 | ... | Key N |  | Value 0 | ... | Value N |
 *  +------+-------+-----+-------+--+---------+-----+---------+
 */
static int __bucket_fixed_create (z_bucket_t *bucket,
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
    nhead->nh_vlength = vlength;
    nhead->nh_items = 0U;
    nhead->nh_free = (bucket->info->size - __NODE_HEAD_SIZE);

    return(0);
}

static int __bucket_fixed_open (z_bucket_t *bucket,
                                uint16_t magic)
{

    struct node_head *nhead  = __NODE_HEAD(bucket);

    if (nhead->nh_crc != __bucket_crc(bucket))
        return(1);

    return(0);
}

static int __bucket_fixed_close (z_bucket_t *bucket) {
    struct node_head *nhead  = __NODE_HEAD(bucket);

    if (nhead->nh_crc == 0)
        nhead->nh_crc = __bucket_crc(bucket);

    return(0);
}

static int __bucket_fixed_search (const z_bucket_t *bucket,
                                  z_bucket_item_t *item,
                                  const void *key,
                                  uint32_t klength)
{
    unsigned int index;
    uint8_t *rkey;

    rkey = z_bsearch_index(key, __NODE_DATA(bucket),
                           __NODE_ITEMS(bucket),
                           __NODE_KLENGTH(bucket),
                           __bucket_fixed_key_cmp,
                           (void *)bucket,
                           &index);

    if (rkey == NULL)
        return(-1);

    item->klength = __NODE_KLENGTH(bucket);
    item->vlength = __NODE_VLENGTH(bucket);
    item->key = (const uint8_t *)rkey;
    item->value = __NODE_END_DATA(bucket) - ((index + 1) * item->vlength);

    return(0);
}

static int __bucket_fixed_append (z_bucket_t *bucket,
                                  const void *key,
                                  uint32_t klength,
                                  const void *value,
                                  uint32_t vlength)
{
    struct node_head *nhead;
    uint8_t *dst;

    /* TODO:
     * assert(nhead->nh_klength == klength);
     * assert(nhead->nh_vlength == vlength);
     */

    nhead = __NODE_HEAD(bucket);

    /* Insert item Key */
    dst = __NODE_DATA(bucket) + (nhead->nh_items * nhead->nh_klength);
    z_memcpy(dst, key, nhead->nh_klength);

    /* Insert item Value */
    dst = __NODE_END_DATA(bucket) - ((1 + nhead->nh_items) * nhead->nh_vlength);
    z_memcpy(dst, value, nhead->nh_vlength);

    /* Update node stats */
    nhead->nh_free -= (nhead->nh_klength + nhead->nh_vlength);
    nhead->nh_items++;
    nhead->nh_crc = 0;

    return(0);
}

static uint32_t __bucket_fixed_avail (const z_bucket_t *bucket) {
    return(__NODE_FREE(bucket));
}

z_bucket_plug_t z_bucket_fixed = {
    .create = __bucket_fixed_create,
    .open   = __bucket_fixed_open,
    .close  = __bucket_fixed_close,

    .search = __bucket_fixed_search,
    .append = __bucket_fixed_append,

    .avail  = __bucket_fixed_avail,
};

