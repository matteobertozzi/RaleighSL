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

#include <zcl/memcpy.h>
#include <zcl/bucket.h>

#define __NODE_HEAD_SIZE            (sizeof(struct node_head))

#define __NODE_HEAD(bucket)         ((struct node_head *)((bucket)->data))
#define __NODE_DATA(bucket)         ((bucket)->data + __NODE_HEAD_SIZE)
#define __NODE_END_DATA(bucket)     ((bucket)->data + (bucket)->info->size)
#define __NODE_VLENGTH(bucket)      (__NODE_HEAD(bucket)->nh_vlength)
#define __NODE_OFFSET(bucket)       (__NODE_HEAD(bucket)->nh_offset)
#define __NODE_ITEMS(bucket)        (__NODE_HEAD(bucket)->nh_items)
#define __NODE_LEVEL(bucket)        (__NODE_HEAD(bucket)->nh_level)

#define __read_u32(dst, src)        z_memcpy(dst, src, sizeof(uint32_t))
#define __write_u32(dst, src)       z_memcpy(dst, src, sizeof(uint32_t))

#define __bucket_crc(bucket)                                                \
    (bucket)->info->crc_func((bucket->info)->user_data,                     \
                             __NODE_DATA(bucket) + sizeof(uint32_t),        \
                             (bucket)->info->size - sizeof(uint32_t))

struct node_head {
    uint32_t    nh_crc;

    uint16_t    nh_magic;
    uint16_t    nh_level;
    uint32_t    nh_vlength;

    uint32_t    nh_items;
    uint32_t    nh_offset;
} __attribute__((packed));

/* ==========================================================================
 *  Bucket Plugin Methods
 *  +------+-------+-------+----+-------+-------+--+---------+----+---------+
 *  | Head | K0Len | Key 0 | .. | KNLen | Key N |  | Value N | .. | Value 0 |
 *  +------+-------+-------+----+-------+-------+--+---------+----+---------+
 */
static int __bucket_fixed_value_create (z_bucket_t *bucket,
                                        uint16_t magic,
                                        uint16_t level,
                                        uint32_t klength,
                                        uint32_t vlength)
{
    struct node_head *nhead = __NODE_HEAD(bucket);

    nhead->nh_crc = 0;
    nhead->nh_magic = magic;
    nhead->nh_level = level;
    nhead->nh_vlength = vlength;
    nhead->nh_items = 0U;
    nhead->nh_offset = 0;

    return(0);
}

static int __bucket_fixed_value_open (z_bucket_t *bucket,
                                      uint16_t magic)
{

    struct node_head *nhead;

    nhead = __NODE_HEAD(bucket);
    if (nhead->nh_crc != __bucket_crc(bucket))
        return(1);

    return(0);
}

static int __bucket_fixed_value_close (z_bucket_t *bucket) {
    struct node_head *nhead  = __NODE_HEAD(bucket);

    if (nhead->nh_crc == 0)
        nhead->nh_crc = __bucket_crc(bucket);

    return(0);
}

static int __bucket_fixed_value_search (const z_bucket_t *bucket,
                                        z_bucket_item_t *item,
                                        const void *key,
                                        uint32_t klength)
{
    z_vcompare_t keycmp;
    uint32_t nitems;
    uint8_t *ivalue;
    uint8_t *ikey;
    void *udata;
    int cmp;

    cmp = 1;
    nitems = __NODE_ITEMS(bucket);
    udata = bucket->info->user_data;
    keycmp = bucket->info->key_compare;

    item->vlength = __NODE_VLENGTH(bucket);

    ikey = __NODE_DATA(bucket);
    ivalue = __NODE_END_DATA(bucket) - item->vlength;

    while (nitems--) {
        __read_u32(&(item->klength), ikey); ikey += sizeof(uint32_t);
        item->key = ikey;                   ikey += item->klength;
        item->value = ivalue;               ivalue -= item->vlength;

        if ((cmp = keycmp(udata, item->key, item->klength, key, klength)) >= 0)
            break;
    }

    return(cmp);
}

static int __bucket_fixed_value_append (z_bucket_t *bucket,
                                        const void *key,
                                        uint32_t klength,
                                        const void *value,
                                        uint32_t vlength)
{
    struct node_head *nhead;
    uint8_t *dst;

    nhead  = __NODE_HEAD(bucket);
    dst = __NODE_DATA(bucket) + __NODE_OFFSET(bucket);

    /* Insert item Key */
    __write_u32(dst, &klength); dst += sizeof(uint32_t);
    z_memcpy(dst, key, klength);

    /* Insert item Value */
    dst = __NODE_END_DATA(bucket) - ((1 + nhead->nh_items) * nhead->nh_vlength);
    z_memcpy(dst, value, nhead->nh_vlength);

    /* Update node stats */
    nhead->nh_offset += klength + sizeof(uint32_t);
    nhead->nh_items++;
    nhead->nh_crc = 0;

    return(0);
}

static uint32_t __bucket_fixed_value_avail (const z_bucket_t *bucket) {
    return(bucket->info->size - __NODE_HEAD_SIZE - __NODE_OFFSET(bucket));
}

z_bucket_plug_t z_bucket_fixed_value = {
    .create = __bucket_fixed_value_create,
    .open   = __bucket_fixed_value_open,
    .close  = __bucket_fixed_value_close,

    .search = __bucket_fixed_value_search,
    .append = __bucket_fixed_value_append,

    .avail  = __bucket_fixed_value_avail,
};

