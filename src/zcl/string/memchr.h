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

#ifndef _Z_MEMCHR_H_
#define _Z_MEMCHR_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <stdint.h>

void *  z_memchr      (const void *s, uint8_t c, unsigned int n);
void *  z_memchr8     (const void *s, uint8_t c, unsigned int n);
void *  z_memchr16    (const void *s, uint8_t c, unsigned int n);
void *  z_memchr32    (const void *s, uint8_t c, unsigned int n);
void *  z_memchr64    (const void *s, uint8_t c, unsigned int n);


void *  z_memrchr     (const void *s, uint8_t c, unsigned int n);
void *  z_memrchr8    (const void *s, uint8_t c, unsigned int n);
void *  z_memrchr16   (const void *s, uint8_t c, unsigned int n);
void *  z_memrchr32   (const void *s, uint8_t c, unsigned int n);
void *  z_memrchr64   (const void *s, uint8_t c, unsigned int n);

void *  z_memtok      (const void *s, const char *delim, unsigned int n);

__Z_END_DECLS__

#endif /* !_Z_MEMCHR_H_ */

