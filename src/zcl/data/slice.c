/*
 *   Copyright 2012 Matteo Bertozzi
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

#include <zcl/slice.h>

typedef uint64_t (*slice_fetch64_t) (const z_slice_t *, unsigned int);
typedef uint32_t (*slice_fetch32_t) (const z_slice_t *, unsigned int);
typedef uint16_t (*slice_fetch16_t) (const z_slice_t *, unsigned int);

#define __slice_fetch64_func(slice)                                     \
    (Z_VTABLE_HAS_METHOD(z_slice_t, slice, fetch64) ?                   \
        Z_VTABLE_METHOD(z_slice_t, slice, fetch64) :                    \
        _z_slice_fetch64)

/* TODO: FIX LSB/MSB */
uint16_t _z_slice_fetch16 (const z_slice_t *slice, unsigned int offset) {
    offset <<= 1;
    return((z_slice_fetch8(slice, offset + 0) << 8) +
            z_slice_fetch8(slice, offset + 1));
}

uint32_t _z_slice_fetch32 (const z_slice_t *slice, unsigned int offset) {
    offset <<= 1;
    return((_z_slice_fetch16(slice, offset + 0) << 16) +
            _z_slice_fetch16(slice, offset + 1));
}

uint64_t _z_slice_fetch64 (const z_slice_t *slice, unsigned int offset) {
    offset <<= 1;
    return(((uint64_t)_z_slice_fetch32(slice, offset + 0) << 32ULL) +
            _z_slice_fetch32(slice, offset + 1));
}

int z_slice_compare (const z_slice_t *a, const z_slice_t *b) {
    slice_fetch64_t afetch64;
    slice_fetch64_t bfetch64;
    unsigned int min_len;
    unsigned int offset;
    unsigned int alen;
    unsigned int blen;
    
    alen = z_slice_length(a);
    blen = z_slice_length(b);
    min_len = (alen < blen) ? alen : blen;
    
    afetch64 = __slice_fetch64_func(a);
    bfetch64 = __slice_fetch64_func(b);
    
    for (offset = 0; min_len > 8; min_len -= 8, ++offset) {
        uint64_t a64, b64;
        
        if ((a64 = afetch64(a, offset)) != (b64 = bfetch64(b, offset))) {
            const unsigned char *m1 = (const unsigned char *)&a64;
            const unsigned char *m2 = (const unsigned char *)&b64;
            if (m1[0] != m2[0]) return(m1[0] - m2[0]);  
            if (m1[1] != m2[1]) return(m1[1] - m2[1]);  
            if (m1[2] != m2[2]) return(m1[2] - m2[2]);  
            if (m1[3] != m2[3]) return(m1[3] - m2[3]);  
            if (m1[4] != m2[4]) return(m1[4] - m2[4]);  
            if (m1[5] != m2[5]) return(m1[5] - m2[5]);  
            if (m1[6] != m2[6]) return(m1[6] - m2[6]);  
            return(m1[7] - m2[7]);        
        }
    }
    
    for (offset = offset << 3; min_len-- > 0; ++offset) {
        uint8_t a8, b8;
        
        if ((a8 = z_slice_fetch8(a, offset)) != (b8 = z_slice_fetch8(b, offset)))
            return(a8 - b8);
    }
    
    return(alen - blen);
}
