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

#include <zcl/base64.h>

#define __BASE64_LINE_SIZE                      76
#define __BASE64_BLOCKS_PER_LINE                (__BASE64_LINE_SIZE >> 2)

/**
 * This array is a lookup table that translates 6-bit positive integer
 * index values into their "Base64 Alphabet" equivalents as specified 
 * in Table 1 of RFC 2045.
 */
static const char __b64_encode[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
};

/**
 * Translation Table to decode
 */
static const char __b64_decode[] = {
    62,  -1,  -1,  -1,  63,  52,  53,  54,  55,  56,  57,  58,  59,  
    60,  61,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   0,   1,   2,   3,   
     4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16, 
    17,  18,  19,  20,  21,  22,  23,  24,  25,  -1,  -1,  -1,  -1,  
    -1,  -1,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36, 
    37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  
    50,  51
};

static void __base64_encode_block (const unsigned char in[3],
                                   unsigned char out[4],
                                   unsigned int length)
{
    out[0] = __b64_encode[in[0] >> 2];
    out[1] = __b64_encode[((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4)];

    switch (length) {
        case 2:
            out[2] = __b64_encode[((in[1] & 0xf) << 2) | ((in[2] & 0xc0) >> 6)];
            out[3] = '=';
            break;
        case 3:
            out[2] = __b64_encode[((in[1] & 0xf) << 2) | ((in[2] & 0xc0) >> 6)];
            out[3] = __b64_encode[in[2] & 0x3f];
            break;
        default:
            out[2] = '=';
            out[3] = '=';
            break;
    }
}

static void __base64_decode_block (const unsigned char in[4],
                                   unsigned char out[3])
{
    out[0] = (in[0] << 2 | in[1] >> 4);
    out[1] = (in[1] << 4 | in[2] >> 2);
    out[2] = (((in[2] << 6) & 0xc0) | in[3]);
}

unsigned int z_base64_encode (void *dst, 
                              const void *src, 
                              unsigned int size)
{
    const unsigned char *psrc = (const unsigned char *)src;
    unsigned char *pdst = (unsigned char *)dst;
    unsigned int nblocks;
    unsigned int block;

    nblocks = 0;
    while (size > 0) {
        block = (size > 3) ? 3 : size;
        __base64_encode_block(psrc, pdst, block);

        nblocks++;
        size -= block;
        psrc += block;
        pdst += 4;

        if (nblocks >= __BASE64_BLOCKS_PER_LINE) {
            *pdst++ = '\r';
            *pdst++ = '\n';
            nblocks = 0;
        }
    }

    return(pdst - ((unsigned char *)dst));
}

unsigned int z_base64_decode (void *dst,
                              const void *src,
                              unsigned int size)
{
    const unsigned char *psrc = (const unsigned char *)src;
    unsigned char *pdst = (unsigned char *)dst;
    unsigned char in[4] = {0, 0, 0, 0};
    unsigned int n;
    char v;

    n = 0;
    while (size > 0) {
        v = (*psrc < 43 || *psrc > 122) ? -1 : __b64_decode[*psrc - 43];       
        if (v != -1)
            in[n++] = v & 0xff;

        if (n == 4) {
            __base64_decode_block(in, pdst);
            in[0] = in[1] = in[2] = in[3] = 0;
            pdst += 3;
            n = 0;
        }

        size--;
        psrc++;
    }

    if (n > 0) {
        __base64_decode_block(in, pdst);
        pdst += n - 1;
    }

    return(pdst - ((unsigned char *)dst));
}

