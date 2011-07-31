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

#ifndef _Z_MEMSWAP_H_
#define _Z_MEMSWAP_H_

void    z_memswap      (void *a, void *b, unsigned int n);
void    z_memswap8     (void *a, void *b, unsigned int n);
void    z_memswap16    (void *a, void *b, unsigned int n);
void    z_memswap32    (void *a, void *b, unsigned int n);
void    z_memswap64    (void *a, void *b, unsigned int n);

#endif /* !_Z_MEMSWAP_H_ */

