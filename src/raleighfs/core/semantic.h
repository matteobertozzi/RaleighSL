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

#ifndef _RALEIGHFS_SEMANTIC_H_
#define _RALEIGHFS_SEMANTIC_H_

#include <raleighfs/types.h>

raleighfs_errno_t   raleighfs_semantic_touch    (raleighfs_t *fs,
                                                 raleighfs_object_plug_t *plug,
                                                 const z_slice_t *blob);
raleighfs_errno_t   raleighfs_semantic_create   (raleighfs_t *fs,
                                                 raleighfs_object_t *object,
                                                 raleighfs_object_plug_t *plug,
                                                 const z_slice_t *name);
raleighfs_errno_t   raleighfs_semantic_open     (raleighfs_t *fs,
                                                 raleighfs_object_t *object,
                                                 const z_slice_t *name);
raleighfs_errno_t   raleighfs_semantic_close    (raleighfs_t *fs,
                                                 raleighfs_object_t *object);
raleighfs_errno_t   raleighfs_semantic_sync     (raleighfs_t *fs,
                                                 raleighfs_object_t *object);
raleighfs_errno_t   raleighfs_semantic_unlink   (raleighfs_t *fs,
                                                 raleighfs_object_t *object);

raleighfs_errno_t   raleighfs_semantic_exists   (raleighfs_t *fs,
                                                 const z_slice_t *name);

raleighfs_errno_t   raleighfs_semantic_rename   (raleighfs_t *fs,
                                                 const z_slice_t *old_name,
                                                 const z_slice_t *new_name);

#endif /* !_RALEIGHFS_SEMANTIC_H_ */

