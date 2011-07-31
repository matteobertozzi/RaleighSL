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

#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#include <zcl/strtol.h>

int z_strtoi32 (const char *str, int base, int32_t *value) {
    char *endptr = NULL;
    long int i32;

    while (isspace(*str)) str++;
    i32 = strtol(str, &endptr, base);
    if (errno == ERANGE)
        return(0);

    if (isspace(*endptr) || (*endptr == '\0' && endptr != str)) {
        *value = (int32_t)i32;
        return(1);
    }

    return(0);
}

int z_strtou32 (const char *str, int base, uint32_t *value) {
    unsigned long int u32;
    char *endptr = NULL;

    while (isspace(*str)) str++;
    u32 = strtoul(str, &endptr, base);
    if (errno == ERANGE)
        return(0);

    if (isspace(*endptr) || (*endptr == '\0' && endptr != str)) {
        *value = (uint32_t)u32;
        return(1);
    }

    return(0);
}

int z_strtoi64 (const char *str, int base, int64_t *value) {
    unsigned long long i64;
    char *endptr = NULL;

    while (isspace(*str)) str++;
    i64 = strtoll(str, &endptr, base);
    if (errno == ERANGE)
        return(0);

    if (isspace(*endptr) || (*endptr == '\0' && endptr != str)) {
        *value = (int64_t)i64;
        return(1);
    }

    return(0);
}

int z_strtou64 (const char *str, int base, uint64_t *value) {
    unsigned long long u64;
    char *endptr = NULL;

    while (isspace(*str)) str++;
    u64 = strtoull(str, &endptr, base);
    if (errno == ERANGE)
        return(0);

    if (isspace(*endptr) || (*endptr == '\0' && endptr != str)) {
        *value = (uint64_t)u64;
        return(1);
    }

    return(0);
}

int z_u64tostr (uint64_t value, char *nptr, unsigned int nptrsize, int base) {
    char buffer[64];
    unsigned int i;
    unsigned int n;
    int c;

    if (value == 0) {
        if (nptrsize < 2)
            return(-2);

        nptr[0] = '0';
        nptr[1] = '\0';
        return(1);
    } 

    for (n = 0; value != 0; ++n) {
        c = (value % base) & 0xFF;
        value /= base;
        buffer[n] = (c > 9) ? (c - 10 + 'a') : (c + '0');
    }

    if (nptrsize < n)
        return(-1);

    n--;
    for (i = 0; i <= n; ++i)
        nptr[i] = buffer[n - i];
    nptr[i] = '\0';

    return(n + 1);
}

int z_u32tostr (uint32_t u32, char *nptr, unsigned int nptrsize, int base) {
    return(z_u64tostr((uint64_t)u32, nptr, nptrsize, base));
}

int z_u16tostr (uint16_t u16, char *nptr, unsigned int nptrsize, int base) {
    return(z_u64tostr((uint64_t)u16, nptr, nptrsize, base));
}

int z_u8tostr (uint8_t u8, char *nptr, unsigned int nptrsize, int base) {
    return(z_u64tostr((uint64_t)u8, nptr, nptrsize, base));
}

int z_i64tostr (int64_t i64, char *buffer, unsigned int bufsize, int base) {
    int n = 0;

    if (i64 < 0) {
        i64 = -i64;
        buffer[0] = '-';
        buffer++;
        bufsize--;
        n = 1;
    }

    return(n + z_u64tostr((uint64_t)i64, buffer, bufsize, base));
}

int z_i32tostr (int32_t i32, char *nptr, unsigned int nptrsize, int base) {
    return(z_i64tostr((int64_t)i32, nptr, nptrsize, base));
}

int z_i16tostr (int16_t i16, char *nptr, unsigned int nptrsize, int base) {
    return(z_i64tostr((int64_t)i16, nptr, nptrsize, base));
}

int z_i8tostr (int8_t i8, char *nptr, unsigned int nptrsize, int base) {
    return(z_i64tostr((int64_t)i8, nptr, nptrsize, base));
}


