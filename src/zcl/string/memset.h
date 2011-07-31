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

#ifndef _Z_MEMSET_H_
#define _Z_MEMSET_H_

#include <stdint.h>

void *  z_memset      (void *dest, int c, unsigned int n);
void *  z_memset8     (void *dest, uint8_t c, unsigned int n);
void *  z_memset16    (void *dest, uint8_t c, unsigned int n);
void *  z_memset32    (void *dest, uint8_t c, unsigned int n);
void *  z_memset64    (void *dest, uint8_t c, unsigned int n);

void *  z_memset_p8   (void *dest, uint8_t pattern, unsigned int n);
void *  z_memset_p16  (void *dest, uint16_t pattern, unsigned int n);
void *  z_memset_p32  (void *dest, uint32_t pattern, unsigned int n);
void *  z_memset_p64  (void *dest, uint64_t pattern, unsigned int n);

void *  z_memzero     (void *dest, unsigned int n);
void *  z_memzero8    (void *dest, unsigned int n);
void *  z_memzero16   (void *dest, unsigned int n);
void *  z_memzero32   (void *dest, unsigned int n);
void *  z_memzero64   (void *dest, unsigned int n);

#endif /* !_Z_MEMSET_H_ */

