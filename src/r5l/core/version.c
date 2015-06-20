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

#include <r5l/version.h>

const char *r5l_info_name(void) { return R5L_NAME; }
uint32_t    r5l_info_version(void) { return R5L_VERSION; }
const char *r5l_info_version_str(void) { return R5L_VERSION_STR; }
const char *r5l_info_build_number (void) { return R5L_BUILD_NR; }
const char *r5l_info_git_rev (void) { return R5L_GIT_REV; }

void r5l_info_dump (FILE *stream) {
  fprintf(stream, "%s %s (build %s git-rev %s)\n",
                  R5L_NAME, R5L_VERSION_STR, R5L_BUILD_NR, R5L_GIT_REV);
}
