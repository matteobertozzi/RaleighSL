/*
 *   Copyright 2011-2013 Matteo Bertozzi
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

#include <zcl/coding.h>

/* ============================================================================
 *  Bytes Required to encode an integer
 */
unsigned int z_uint32_bytes (uint32_t value) {
    if (value < (1ul <<  8)) return(1);
    if (value < (1ul << 16)) return(2);
    if (value < (1ul << 24)) return(3);
    return(4);
}

unsigned int z_uint64_bytes (uint64_t value) {
    if (value < (1ull << 32)) {
        if (value < (1ull <<  8)) return(1);
        if (value < (1ull << 16)) return(2);
        if (value < (1ull << 24)) return(3);
        return(4);
    } else {
        if (value < (1ull << 40)) return(5);
        if (value < (1ull << 48)) return(6);
        if (value < (1ull << 56)) return(7);
        return(8);
    }
}

/* ============================================================================
 *  Encode/Decode variable-length
 */
unsigned int z_vint_bytes (uint64_t value) {
    if (value < (1ull << 35)) {
        if (value < (1ull <<  7)) return(1);
        if (value < (1ull << 14)) return(2);
        if (value < (1ull << 21)) return(3);
        if (value < (1ull << 28)) return(4);
        return(5);
    } else {
        if (value < (1ull << 42)) return(6);
        if (value < (1ull << 49)) return(7);
        if (value < (1ull << 56)) return(8);
        if (value < (1ull << 63)) return(9);
        return(10);
    }
}

int z_encode_vint (unsigned char *buf, uint64_t value) {
    int size = 1;
    while (value >= 128) {
        *buf++ = (value & 0x7f) | 128;
        value >>= 7;
        size++;
    }
    *buf = value & 0x7f;
    return(size);
}

int z_decode_vint (const unsigned char *buf,
                   unsigned int buflen,
                   uint64_t *value)
{
    unsigned int shift = 0;
    uint64_t result = 0;
    int count = 0;

    while (shift <= 63 && count++ < buflen) {
        uint64_t b = *buf++;
        if (b & 128) {
            result |= (b & 0x7f) << shift;
        } else {
            result |= (b << shift);
            return(count);
        }
        shift += 7;
    }

    return(-1);
}

/* ============================================================================
 *  Encode/Decode unsigned integer
 */
void z_encode_uint (unsigned char *buf, unsigned int length, uint64_t value) {
    switch (length) {
        case 8: buf[7] = (value >> 56) & 0xff;
        case 7: buf[6] = (value >> 48) & 0xff;
        case 6: buf[5] = (value >> 40) & 0xff;
        case 5: buf[4] = (value >> 32) & 0xff;
        case 4: buf[3] = (value >> 24) & 0xff;
        case 3: buf[2] = (value >> 16) & 0xff;
        case 2: buf[1] = (value >>  8) & 0xff;
        case 1: buf[0] = value & 0xff;
    }
}

void z_decode_uint16 (const unsigned char *buffer,
                      unsigned int length, uint16_t *value)
{
    uint32_t result = 0;

    switch (length) {
        case 2: result += buffer[1] <<  8;
        case 1: result += buffer[0];
    }

    *value = result;
}

void z_decode_uint32 (const unsigned char *buffer,
                      unsigned int length,
                      uint32_t *value)
{
    uint32_t result = 0;

    switch (length) {
        case 4: result += ((uint64_t)buffer[3]) << 24;
        case 3: result += buffer[2] << 16;
        case 2: result += buffer[1] <<  8;
        case 1: result += buffer[0];
    }

    *value = result;
}

void z_decode_uint64 (const unsigned char *buffer,
                      unsigned int length,
                      uint64_t *value)
{
    uint64_t result = 0;

    switch (length) {
        case 8: result  = ((uint64_t)buffer[7]) << 56;
        case 7: result += ((uint64_t)buffer[6]) << 48;
        case 6: result += ((uint64_t)buffer[5]) << 40;
        case 5: result += ((uint64_t)buffer[4]) << 32;
        case 4: result += ((uint64_t)buffer[3]) << 24;
        case 3: result += buffer[2] << 16;
        case 2: result += buffer[1] <<  8;
        case 1: result += buffer[0];
    }

    *value = result;
}