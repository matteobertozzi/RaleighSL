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

#ifndef _RALEIGHFS_EXECUTE_H_
#define _RALEIGHFS_EXECUTE_H_

#include <raleighfs/types.h>

#define RALEIGHFS_SEMANTIC_MSG(x)       (!((x) & ((1 << 3) << 26)))
#define RALEIGHFS_OBJECT_MSG(x)         (!!((x) & ((1 << 3) << 26)))
#define RALEIGHFS_MSG_TYPE(x)           ((x) & (15 << 26)) // 7 << 26

#define RALEIGHFS_CREATE                (((0 << 3) | 1) << 26)
#define RALEIGHFS_RENAME                (((0 << 3) | 2) << 26)
#define RALEIGHFS_EXISTS                (((0 << 3) | 3) << 26)

#define RALEIGHFS_QUERY                 (((1 << 3) | 1) << 26)
#define RALEIGHFS_INSERT                (((1 << 3) | 2) << 26)
#define RALEIGHFS_UPDATE                (((1 << 3) | 3) << 26)
#define RALEIGHFS_REMOVE                (((1 << 3) | 4) << 26)
#define RALEIGHFS_IOCTL                 (((1 << 3) | 5) << 26)
#define RALEIGHFS_SYNC                  (((1 << 3) | 6) << 26)
#define RALEIGHFS_UNLINK                (((1 << 3) | 7) << 26)

raleighfs_errno_t   raleighfs_execute   (raleighfs_t *fs,
                                         const z_rdata_t *object_name,
                                         z_message_t *msg);

#endif /* !_RALEIGHFS_EXECUTE_H_ */

