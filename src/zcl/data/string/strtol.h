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

#ifndef _Z_STRTOL_H_
#define _Z_STRTOL_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <stdint.h>

int     z_strtoi32   (const char *str, int base, int32_t *value);
int     z_strtou32   (const char *str, int base, uint32_t *value);

int     z_strtoi64   (const char *str, int base, int64_t *value);
int     z_strtou64   (const char *str, int base, uint64_t *value);

int     z_u64tostr   (uint64_t u64, char *nptr, unsigned int nptrsize, int base);
int     z_u32tostr   (uint32_t u32, char *nptr, unsigned int nptrsize, int base);
int     z_u16tostr   (uint16_t u16, char *nptr, unsigned int nptrsize, int base);
int     z_u8tostr    (uint8_t u8, char *nptr, unsigned int nptrsize, int base);

int     z_i64tostr   (int64_t i64, char *nptr, unsigned int nptrsize, int base);
int     z_i32tostr   (int32_t i32, char *nptr, unsigned int nptrsize, int base);
int     z_i16tostr   (int16_t i16, char *nptr, unsigned int nptrsize, int base);
int     z_i8tostr    (int8_t i8, char *nptr, unsigned int nptrsize, int base);

__Z_END_DECLS__

#endif /* !_Z_STRTOL_H_ */
