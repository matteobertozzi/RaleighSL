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

#include <zcl/bucket.h>
#include <zcl/memcpy.h>

int z_bucket_create (z_bucket_t *bucket,
                     z_bucket_plug_t *plug,
                     z_bucket_info_t *info,
                     uint8_t *data,
                     uint16_t magic,
                     uint16_t level,
                     uint32_t klength,
                     uint32_t vlength)
{
    bucket->info = info;
    bucket->plug = plug;
    bucket->data = data;
    return(plug->create(bucket, magic, level, klength, vlength));
}

int z_bucket_open (z_bucket_t *bucket,
                   z_bucket_plug_t *plug,
                   z_bucket_info_t *info,
                   uint8_t *data,
                   uint16_t magic)
{
    bucket->info = info;
    bucket->plug = plug;
    bucket->data = data;
    return(plug->open(bucket, magic));
}

int z_bucket_close (z_bucket_t *bucket) {
    return(bucket->plug->close(bucket));
}

int z_bucket_search (const z_bucket_t *bucket,
                     z_bucket_item_t *item,
                     const void *key,
                     uint32_t klength)
{
    return(bucket->plug->search(bucket, item, key, klength));
}

int z_bucket_append (z_bucket_t *bucket,
                     const void *key,
                     uint32_t klength,
                     const void *value,
                     uint32_t vlength)
{
    return(bucket->plug->append(bucket, key, klength, value, vlength));
}

uint32_t z_bucket_avail (const z_bucket_t *bucket) {
    return(bucket->plug->avail(bucket));
}

uint32_t z_bucket_used (const z_bucket_t *bucket) {
    return(bucket->info->size - bucket->plug->avail(bucket));
}

