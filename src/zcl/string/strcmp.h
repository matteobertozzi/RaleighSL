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

#ifndef _Z_STRCMP_H_
#define _Z_STRCMP_H_

int     z_strcmp          (const char *s1, const char *s2);
int     z_strncmp         (const char *s1, const char *s2, unsigned int n);

int     z_strcasecmp      (const char *s1, const char *s2);
int     z_strncasecmp     (const char *s1, const char *s2, unsigned int n);

#define z_streq(s1, s2)         (!z_strcmp(s1, s2))
#define z_strneq(s1, s2, n)     (!z_strcmp(s1, s2, n))

#endif /* !_Z_STRCMP_H_ */

