/*
 *   Copyright 2007-2013 Matteo Bertozzi
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

#ifndef _RALEIGHFS_SSET_H_
#define _RALEIGHFS_SSET_H_

#include <raleighfs/raleighfs.h>
#include <zcl/array.h>
#include <zcl/bytes.h>

extern const raleighfs_object_plug_t raleighfs_object_sset;

raleighfs_errno_t raleighfs_sset_insert (raleighfs_t *fs,
                                         raleighfs_transaction_t *transaction,
                                         raleighfs_object_t *object,
                                         int allow_update,
                                         z_bytes_t *key,
                                         z_bytes_t *value);
raleighfs_errno_t raleighfs_sset_update (raleighfs_t *fs,
                                         raleighfs_transaction_t *transaction,
                                         raleighfs_object_t *object,
                                         z_bytes_t *key,
                                         z_bytes_t *value,
                                         z_bytes_t **old_value);
raleighfs_errno_t raleighfs_sset_remove (raleighfs_t *fs,
                                         raleighfs_transaction_t *transaction,
                                         raleighfs_object_t *object,
                                         const z_bytes_t *key,
                                         z_bytes_t **value);

raleighfs_errno_t raleighfs_sset_get    (raleighfs_t *fs,
                                         raleighfs_transaction_t *transaction,
                                         raleighfs_object_t *object,
                                         const z_bytes_t *key,
                                         z_bytes_t **value);
raleighfs_errno_t raleighfs_sset_scan   (raleighfs_t *fs,
                                         raleighfs_transaction_t *transaction,
                                         raleighfs_object_t *object,
                                         const z_bytes_t *key,
                                         int include_key,
                                         size_t count,
                                         z_array_t *keys,
                                         z_array_t *values);

#endif /* !_RALEIGHFS_SSET_H_ */
