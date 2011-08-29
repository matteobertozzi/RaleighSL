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

#include <zcl/snprintf.h>
#include <zcl/strtol.h>
#include <zcl/strlen.h>
#include <zcl/memcpy.h>
#include <zcl/macros.h>

#include <stdarg.h>

static int __snprint_string (char *buf, unsigned int bufsize, va_list arg) {
    int nchars;
    char *s;

    if ((s = va_arg(arg, char *)) == NULL) {
        nchars = z_min(bufsize, 6);
        z_memcpy(buf, "(none)", nchars);
    } else {
        nchars = z_min(bufsize, z_strlen(s));
        z_memcpy(buf, s, nchars);
    }

    return(nchars);
}

static int __snprint_number (char *buf, unsigned int bufsize,
                             const char *frmt, va_list arg)
{
    int has_sign;
    int nchars;
    int base;

    has_sign = (frmt[0] == 'i');
    switch (frmt[2]) {
        case 'd': base = 10; break;
        case 'x': base = 16; break;
        case 'o': base = 8;  break;
        case 'b': base = 2;  break;
        default:  base = 10; break;
    }

    switch (frmt[1]) {
        case '1':
            if (has_sign)
                nchars = z_i8tostr(va_arg(arg, int), buf, bufsize, base);
            else
                nchars = z_u8tostr(va_arg(arg, int), buf, bufsize, base);
            break;
        case '2':
            if (has_sign)
                nchars = z_i16tostr(va_arg(arg, int), buf, bufsize, base);
            else
                nchars = z_u16tostr(va_arg(arg, int), buf, bufsize, base);
            break;
        case '4':
            if (has_sign)
                nchars = z_i32tostr(va_arg(arg, int32_t), buf, bufsize, base);
            else
                nchars = z_u32tostr(va_arg(arg, uint32_t), buf, bufsize, base);
            break;
        case '8':
            if (has_sign)
                nchars = z_i64tostr(va_arg(arg, int64_t), buf, bufsize, base);
            else
                nchars = z_u64tostr(va_arg(arg, uint64_t), buf, bufsize, base);
            break;
    }

    return(nchars);
}

int z_vsvnprintf (char *buf, unsigned int bufsize,
                  const char *frmt, va_list arg)
{
    unsigned int avail;
    int n, nchars;
    char c;

    if (bufsize == 0)
        return(0);

    bufsize--;      /* Keep 1byte for '\0' */
    nchars = 0;
    while ((c = *frmt++) != '\0' && (avail = (bufsize - nchars)) > 0) {
        if (c != '%') {
            *buf++ = c;
            nchars++;
            continue;
        }

        /* 
         * %% -> %
         * %c -> char
         * %s -> string
         * %u32d -> uint32_t decimal
         * %u16x -> uint16_t hex
         * %i64o -> int64_t octal
         */
        switch ((c = *frmt++)) {
            case 's':
                n = __snprint_string(buf, avail, arg);
                nchars += n;
                buf += n;
                break;
            case 'u':
            case 'i':
                n = __snprint_number(buf, avail, frmt - 1, arg);
                nchars += n;
                frmt += 2;
                buf += n;
                break;
            case 'c':
                *buf++ = va_arg(arg, int);
                nchars++;
                break;
            case '%':
                *buf++ = '%';
                nchars++;
                break;
        }        
    }

    *buf = '\0';
    return(nchars);
}

int z_snprintf (char *buf, unsigned int bufsize, const char *frmt, ...) {
    va_list ap;
    int n;

    va_start(ap, frmt);
    n = z_vsvnprintf(buf, bufsize, frmt, ap);
    va_end(ap);

    return(n);
}

