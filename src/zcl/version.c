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

#include <zcl/version.h>

const char *zcl_info_name(void) { return ZCL_NAME; }
uint32_t    zcl_info_version(void) { return ZCL_VERSION; }
const char *zcl_info_version_str(void) { return ZCL_VERSION_STR; }
const char *zcl_info_build_number (void) { return ZCL_BUILD_NR; }
const char *zcl_info_git_rev (void) { return ZCL_GIT_REV; }

void zcl_info_dump (FILE *stream) {
  fprintf(stream, "%s %s (build %s git-rev %s)\n",
                  ZCL_NAME, ZCL_VERSION_STR, ZCL_BUILD_NR, ZCL_GIT_REV);
}