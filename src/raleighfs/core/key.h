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

#ifndef _RALEIGHFS_KEY_H_
#define _RALEIGHFS_KEY_H_

#include <raleighfs/types.h>

raleighfs_errno_t   raleighfs_key_object        (raleighfs_t *fs,
                                                 raleighfs_key_t *key,
                                                 const z_rdata_t *name);

int                 raleighfs_key_compare       (raleighfs_t *fs,
                                                 const raleighfs_key_t *a,
                                                 const raleighfs_key_t *b);

#endif /* !_RALEIGHFS_KEY_H_ */

