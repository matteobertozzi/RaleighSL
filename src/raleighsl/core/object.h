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

#ifndef _RALEIGHSL_OBJECT_H_
#define _RALEIGHSL_OBJECT_H_

#include <raleighsl/types.h>

raleighsl_errno_t raleighsl_object_create (raleighsl_t *fs,
                                           const raleighsl_object_plug_t *plug,
                                           uint64_t oid);
raleighsl_errno_t raleighsl_object_open   (raleighsl_t *fs,
                                           raleighsl_object_t *object);
raleighsl_errno_t raleighsl_object_close  (raleighsl_t *fs,
                                           raleighsl_object_t *object);
raleighsl_errno_t raleighsl_object_sync   (raleighsl_t *fs,
                                           raleighsl_object_t *object);

#define raleighsl_object_is_open(fs, object)                  \
  ((object)->membufs != NULL || (object)->devbufs != NULL)

raleighsl_object_t *raleighsl_obj_cache_get     (raleighsl_t *fs,
                                                 uint64_t oid);
void                raleighsl_obj_cache_release (raleighsl_t *fs,
                                                 raleighsl_object_t *object);

#endif /* !_RALEIGHSL_OBJECT_H_ */
