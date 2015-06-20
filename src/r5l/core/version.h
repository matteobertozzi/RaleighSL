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

#ifndef _R5L_VERSION_H_
#define _R5L_VERSION_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <stdio.h>

const char *r5l_info_name(void);
uint32_t    r5l_info_version(void);
const char *r5l_info_version_str(void);
const char *r5l_info_build_number (void);
const char *r5l_info_git_rev (void);

void        r5l_info_dump (FILE *stream);

__Z_END_DECLS__

#endif /* !_R5L_VERSION_H_ */
