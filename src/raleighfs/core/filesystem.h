/*
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

#ifndef _RALEIGHFS_FILESYSTEM_H_
#define _RALEIGHFS_FILESYSTEM_H_

#include <raleighfs/types.h>

/* ============================================================================
 *  File-system related
 */
raleighfs_t *       raleighfs_alloc     (raleighfs_t *fs);
void                raleighfs_free      (raleighfs_t *fs);

raleighfs_errno_t   raleighfs_create    (raleighfs_t *fs,
                                         raleighfs_device_t *device,
                                         const raleighfs_format_plug_t *format,
                                         const raleighfs_space_plug_t *space,
                                         const raleighfs_semantic_plug_t *semantic);
raleighfs_errno_t   raleighfs_open      (raleighfs_t *fs,
                                         raleighfs_device_t *device);

raleighfs_errno_t   raleighfs_close     (raleighfs_t *fs);
raleighfs_errno_t   raleighfs_sync      (raleighfs_t *fs);

/* ============================================================================
 *  Plugins register/unregister methods
 */
raleighfs_errno_t raleighfs_plug_object     (raleighfs_t *fs,
                                             const raleighfs_object_plug_t *plug);
raleighfs_errno_t raleighfs_unplug_object   (raleighfs_t *fs,
                                             const raleighfs_object_plug_t *plug);

raleighfs_errno_t raleighfs_plug_semantic   (raleighfs_t *fs,
                                             const raleighfs_semantic_plug_t *plug);
raleighfs_errno_t raleighfs_unplug_semantic (raleighfs_t *fs,
                                             const raleighfs_semantic_plug_t *plug);

raleighfs_errno_t raleighfs_plug_format     (raleighfs_t *fs,
                                             const raleighfs_format_plug_t *plug);
raleighfs_errno_t raleighfs_unplug_format   (raleighfs_t *fs,
                                             const raleighfs_format_plug_t *plug);

raleighfs_errno_t raleighfs_plug_space      (raleighfs_t *fs,
                                             const raleighfs_space_plug_t *plug);
raleighfs_errno_t raleighfs_unplug_space    (raleighfs_t *fs,
                                             const raleighfs_space_plug_t *plug);

/* ============================================================================
 *  Plugins lookup by label
 */
const raleighfs_object_plug_t *  raleighfs_object_plug_lookup (raleighfs_t *fs,
                                                               const z_byte_slice_t *label);
const raleighfs_semantic_plug_t *raleighfs_semantic_plug_lookup (raleighfs_t *fs,
                                                                 const z_byte_slice_t *label);
const raleighfs_space_plug_t *   raleighfs_space_plug_lookup (raleighfs_t *fs,
                                                              const z_byte_slice_t *label);
const raleighfs_format_plug_t *  raleighfs_format_plug_lookup (raleighfs_t *fs,
                                                               const z_byte_slice_t *label);

#endif /* !_RALEIGHFS_FILESYSTEM_H_ */
