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

#ifndef _Z_BUCKET_H_
#define _Z_BUCKET_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/checksum.h>
#include <zcl/types.h>

#include <stdint.h>

Z_TYPEDEF_CONST_STRUCT(z_bucket_plug)
Z_TYPEDEF_STRUCT(z_bucket_item)
Z_TYPEDEF_STRUCT(z_bucket_info)
Z_TYPEDEF_STRUCT(z_bucket)

#define Z_BUCKET(x)             Z_CAST(z_bucket_t, x)

struct z_bucket_plug {
    int         (*create)       (z_bucket_t *bucket,
                                 uint16_t magic,
                                 uint16_t level,
                                 uint32_t klength,
                                 uint32_t vlength);
    int         (*open)         (z_bucket_t *bucket,
                                 uint16_t magic);
    int         (*close)        (z_bucket_t *bucket);

    int         (*search)       (const z_bucket_t *bucket,
                                 z_bucket_item_t *item,
                                 const void *key,
                                 uint32_t klength);
    int         (*append)       (z_bucket_t *bucket,
                                 const void *key,
                                 uint32_t klength,
                                 const void *value,
                                 uint32_t vlength);

    uint32_t    (*avail)        (const z_bucket_t *bucket);
};

struct z_bucket_item {
    uint32_t        klength;
    uint32_t        vlength;
    const uint8_t * key;
    const uint8_t * value;
};

struct z_bucket_info {
    void *          user_data;
    z_vcompare_t    key_compare;
    z_csum32_t      crc_func;
    unsigned int    size;
};

struct z_bucket {
    z_bucket_plug_t *plug;
    z_bucket_info_t *info;
    uint8_t *        data;
};

extern z_bucket_plug_t z_bucket_fixed;
extern z_bucket_plug_t z_bucket_variable;
extern z_bucket_plug_t z_bucket_fixed_key;
extern z_bucket_plug_t z_bucket_fixed_value;

int         z_bucket_create         (z_bucket_t *bucket,
                                     z_bucket_plug_t *plug,
                                     z_bucket_info_t *info,
                                     uint8_t *data,
                                     uint16_t magic,
                                     uint16_t level,
                                     uint32_t klength,
                                     uint32_t vlength);
int         z_bucket_open           (z_bucket_t *bucket,
                                     z_bucket_plug_t *plug,
                                     z_bucket_info_t *info,
                                     uint8_t *data,
                                     uint16_t magic);
int         z_bucket_close          (z_bucket_t *bucket);

int         z_bucket_search         (const z_bucket_t *bucket,
                                     z_bucket_item_t *item,
                                     const void *key,
                                     uint32_t klength);
int         z_bucket_append         (z_bucket_t *bucket,
                                     const void *key,
                                     uint32_t klength,
                                     const void *value,
                                     uint32_t vlength);
uint32_t    z_bucket_avail          (const z_bucket_t *bucket);
uint32_t    z_bucket_used           (const z_bucket_t *bucket);

__Z_END_DECLS__

#endif /* !_Z_BUCKET_H_ */

