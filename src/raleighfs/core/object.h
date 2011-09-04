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

#ifndef _RALEIGHFS_OBJECT_H_
#define _RALEIGHFS_OBJECT_H_

#include <raleighfs/types.h>

raleighfs_errno_t   raleighfs_object_query      (raleighfs_t *fs,
                                                 raleighfs_object_t *object,
                                                 z_message_t *message);
raleighfs_errno_t   raleighfs_object_insert     (raleighfs_t *fs,
                                                 raleighfs_object_t *object,
                                                 z_message_t *message);
raleighfs_errno_t   raleighfs_object_update     (raleighfs_t *fs,
                                                 raleighfs_object_t *object,
                                                 z_message_t *message);
raleighfs_errno_t   raleighfs_object_remove     (raleighfs_t *fs,
                                                 raleighfs_object_t *object,
                                                 z_message_t *message);
raleighfs_errno_t   raleighfs_object_ioctl      (raleighfs_t *fs,
                                                 raleighfs_object_t *object,
                                                 z_message_t *message);

#endif /* !_RALEIGHFS_OBJECT_H_ */

