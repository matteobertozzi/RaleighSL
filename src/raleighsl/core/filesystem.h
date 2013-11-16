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

#ifndef _RALEIGHSL_FILESYSTEM_H_
#define _RALEIGHSL_FILESYSTEM_H_

#include <raleighsl/types.h>

/* ============================================================================
 *  File-system related
 */
raleighsl_t *       raleighsl_alloc     (raleighsl_t *fs);
void                raleighsl_free      (raleighsl_t *fs);

raleighsl_errno_t   raleighsl_create    (raleighsl_t *fs,
                                         raleighsl_device_t *device,
                                         const raleighsl_format_plug_t *format,
                                         const raleighsl_space_plug_t *space,
                                         const raleighsl_semantic_plug_t *semantic);
raleighsl_errno_t   raleighsl_open      (raleighsl_t *fs,
                                         raleighsl_device_t *device);

raleighsl_errno_t   raleighsl_close     (raleighsl_t *fs);
raleighsl_errno_t   raleighsl_sync      (raleighsl_t *fs);

/* ============================================================================
 *  Plugins register/unregister methods
 */
raleighsl_errno_t raleighsl_plug_object     (raleighsl_t *fs,
                                             const raleighsl_object_plug_t *plug);
raleighsl_errno_t raleighsl_unplug_object   (raleighsl_t *fs,
                                             const raleighsl_object_plug_t *plug);

raleighsl_errno_t raleighsl_plug_semantic   (raleighsl_t *fs,
                                             const raleighsl_semantic_plug_t *plug);
raleighsl_errno_t raleighsl_unplug_semantic (raleighsl_t *fs,
                                             const raleighsl_semantic_plug_t *plug);

raleighsl_errno_t raleighsl_plug_format     (raleighsl_t *fs,
                                             const raleighsl_format_plug_t *plug);
raleighsl_errno_t raleighsl_unplug_format   (raleighsl_t *fs,
                                             const raleighsl_format_plug_t *plug);

raleighsl_errno_t raleighsl_plug_space      (raleighsl_t *fs,
                                             const raleighsl_space_plug_t *plug);
raleighsl_errno_t raleighsl_unplug_space    (raleighsl_t *fs,
                                             const raleighsl_space_plug_t *plug);

/* ============================================================================
 *  Plugins lookup by label
 */
const raleighsl_object_plug_t *  raleighsl_object_plug_lookup (raleighsl_t *fs,
                                                               const z_byte_slice_t *label);
const raleighsl_semantic_plug_t *raleighsl_semantic_plug_lookup (raleighsl_t *fs,
                                                                 const z_byte_slice_t *label);
const raleighsl_space_plug_t *   raleighsl_space_plug_lookup (raleighsl_t *fs,
                                                              const z_byte_slice_t *label);
const raleighsl_format_plug_t *  raleighsl_format_plug_lookup (raleighsl_t *fs,
                                                               const z_byte_slice_t *label);

#endif /* !_RALEIGHSL_FILESYSTEM_H_ */
