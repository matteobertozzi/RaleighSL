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

#ifndef _RALEIGHSL_BITMAP_H_
#define _RALEIGHSL_BITMAP_H_

#include <raleighsl/raleighsl.h>

extern const raleighsl_object_plug_t raleighsl_object_bitmap;

raleighsl_errno_t raleighsl_bitmap_test   (raleighsl_t *fs,
                                           const raleighsl_transaction_t *transaction,
                                           raleighsl_object_t *object,
                                           uint64_t offset,
                                           uint64_t count,
                                           int marked);
raleighsl_errno_t raleighsl_bitmap_find   (raleighsl_t *fs,
                                           const raleighsl_transaction_t *transaction,
                                           raleighsl_object_t *object,
                                           uint64_t offset,
                                           uint64_t count,
                                           int marked);
raleighsl_errno_t raleighsl_bitmap_mark   (raleighsl_t *fs,
                                           raleighsl_transaction_t *transaction,
                                           raleighsl_object_t *object,
                                           uint64_t offset,
                                           uint64_t count,
                                           int value);
raleighsl_errno_t raleighsl_bitmap_invert (raleighsl_t *fs,
                                           raleighsl_transaction_t *transaction,
                                           raleighsl_object_t *object,
                                           uint64_t offset,
                                           uint64_t count);
raleighsl_errno_t raleighsl_bitmap_resize (raleighsl_t *fs,
                                           const raleighsl_transaction_t *transaction,
                                           raleighsl_object_t *object,
                                           uint64_t count);

#endif /* !_RALEIGHSL_BITMAP_H_ */
