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

#ifndef _Z_MEMCPY_H_
#define _Z_MEMCPY_H_

void *  z_memcpy      (void *dest, const void *src, unsigned int n);
void *  z_memcpy8     (void *dest, const void *src, unsigned int n);
void *  z_memcpy16    (void *dest, const void *src, unsigned int n);
void *  z_memcpy32    (void *dest, const void *src, unsigned int n);
void *  z_memcpy64    (void *dest, const void *src, unsigned int n);

void *  z_membcpy     (void *dest, const void *src, unsigned int n);
void *  z_membcpy8    (void *dest, const void *src, unsigned int n);
void *  z_membcpy16   (void *dest, const void *src, unsigned int n);
void *  z_membcpy32   (void *dest, const void *src, unsigned int n);
void *  z_membcpy64   (void *dest, const void *src, unsigned int n);

void *  z_memmove     (void *dest, const void *src, unsigned int n);
void *  z_memmove8    (void *dest, const void *src, unsigned int n);
void *  z_memmove16   (void *dest, const void *src, unsigned int n);
void *  z_memmove32   (void *dest, const void *src, unsigned int n);
void *  z_memmove64   (void *dest, const void *src, unsigned int n);

#endif /* !_Z_MEMCPY_H_ */

