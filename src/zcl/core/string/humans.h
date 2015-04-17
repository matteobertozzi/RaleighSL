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

#ifndef _Z_HUMANS_H_
#define _Z_HUMANS_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>

#define Z_KB(n)         ((n) << 10)
#define Z_MB(n)         ((n) << 20)
#define Z_GB(n)         ((n) << 30)
#define Z_TB(n)         ((n) << 40)
#define Z_PB(n)         ((n) << 50)
#define Z_EB(n)         ((n) << 60)

typedef char *(z_human_u64_t) (char *buffer, size_t bufsize, uint64_t x);
typedef char *(z_human_dbl_t) (char *buffer, size_t bufsize, double x);

char *z_human_dops    (char *buffer, size_t bufsize, double n);
char *z_human_dsize   (char *buffer, size_t bufsize, double size);
char *z_human_dtime   (char *buffer, size_t bufsize, double time);

char *z_human_ops     (char *buffer, size_t bufsize, uint64_t n);
char *z_human_size    (char *buffer, size_t bufsize, uint64_t size);
char *z_human_time    (char *buffer, size_t bufsize, uint64_t usec);
char *z_human_date    (char *buffer, size_t bufsize, uint64_t usec);

char *z_human_dblock  (char *buffer, size_t bufsize, const void *data, size_t dsize);

int   z_human_dblock_print (FILE *stream, const void *data, size_t dsize);

__Z_END_DECLS__

#endif /* !_Z_HUMANS_H_ */
