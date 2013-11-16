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

#ifndef _RALEIGHSL_SSET_H_
#define _RALEIGHSL_SSET_H_

#include <raleighsl/raleighsl.h>
#include <zcl/bytesref.h>
#include <zcl/array.h>

extern const raleighsl_object_plug_t raleighsl_object_sset;

raleighsl_errno_t raleighsl_sset_insert (raleighsl_t *fs,
                                         raleighsl_transaction_t *transaction,
                                         raleighsl_object_t *object,
                                         int allow_update,
                                         const z_bytes_ref_t *key,
                                         const z_bytes_ref_t *value);
raleighsl_errno_t raleighsl_sset_update (raleighsl_t *fs,
                                         raleighsl_transaction_t *transaction,
                                         raleighsl_object_t *object,
                                         const z_bytes_ref_t *key,
                                         const z_bytes_ref_t *value,
                                         z_bytes_ref_t *old_value);
raleighsl_errno_t raleighsl_sset_remove (raleighsl_t *fs,
                                         raleighsl_transaction_t *transaction,
                                         raleighsl_object_t *object,
                                         const z_bytes_ref_t *key,
                                         z_bytes_ref_t *value);

raleighsl_errno_t raleighsl_sset_get    (raleighsl_t *fs,
                                         const raleighsl_transaction_t *transaction,
                                         raleighsl_object_t *object,
                                         const z_bytes_ref_t *key,
                                         z_bytes_ref_t *value);
raleighsl_errno_t raleighsl_sset_scan   (raleighsl_t *fs,
                                         const raleighsl_transaction_t *transaction,
                                         raleighsl_object_t *object,
                                         const z_bytes_ref_t *key,
                                         int include_key,
                                         size_t count,
                                         z_array_t *keys,
                                         z_array_t *values);

#endif /* !_RALEIGHSL_SSET_H_ */
