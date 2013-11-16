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

#ifndef _RALEIGHSL_SEMANTIC_H_
#define _RALEIGHSL_SEMANTIC_H_

#include <raleighsl/types.h>

raleighsl_errno_t raleighsl_semantic_create (raleighsl_t *fs,
                                             const raleighsl_object_plug_t *plug,
                                             const z_bytes_ref_t *name,
                                             uint64_t *oid);
raleighsl_errno_t raleighsl_semantic_open   (raleighsl_t *fs,
                                             const z_bytes_ref_t *name,
                                             uint64_t *oid);
raleighsl_errno_t raleighsl_semantic_unlink (raleighsl_t *fs,
                                             const z_bytes_ref_t *name);
raleighsl_errno_t raleighsl_semantic_rename (raleighsl_t *fs,
                                             const z_bytes_ref_t *old_name,
                                             const z_bytes_ref_t *new_name);

#endif /* !_RALEIGHSL_SEMANTIC_H_ */
