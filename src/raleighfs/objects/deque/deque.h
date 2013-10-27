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

#ifndef _RALEIGHFS_DEQUE_H_
#define _RALEIGHFS_DEQUE_H_

#include <raleighfs/raleighfs.h>
#include <zcl/bytes.h>

extern const raleighfs_object_plug_t raleighfs_object_deque;


raleighfs_errno_t raleighfs_deque_push (raleighfs_t *fs,
                                        raleighfs_transaction_t *transaction,
                                        raleighfs_object_t *object,
                                        int push_front,
                                        z_bytes_t *data);
raleighfs_errno_t raleighfs_deque_pop (raleighfs_t *fs,
                                       raleighfs_transaction_t *transaction,
                                       raleighfs_object_t *object,
                                       int pop_front,
                                       z_bytes_t **data);

#endif /* !_RALEIGHFS_DEQUE_H_ */
