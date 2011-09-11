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

int z_vsvnprintf (char *buf, unsigned int bufsize,
                  const char *frmt, va_list ap)
{
    unsigned int avail;
    int n, nchars;
    int base;
    char *s;
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
                if ((s = va_arg(ap, char *)) == NULL) {
                    n = z_min(bufsize, 6);
                    z_memcpy(buf, "(none)", n);
                } else {
                    n = z_min(bufsize, z_strlen(s));
                    z_memcpy(buf, s, n);
                }
                nchars += n;
                buf += n;
                break;
            case 'u':
            case 'i':
                switch (frmt[1]) {
                    case 'd': base = 10; break;
                    case 'x': base = 16; break;
                    case 'o': base = 8;  break;
                    case 'b': base = 2;  break;
                    default:  base = 10; break;
                }

                switch (frmt[0]) {
                  case '1':
                  case '2':
                  case '4':
                     if (c == 'i')
                         n = z_i32tostr(va_arg(ap, int32_t), buf, avail, base);
                     else
                         n = z_u32tostr(va_arg(ap, uint32_t), buf, avail, base);
                     break;
                  case '8':
                     if (c == 'i')
                         n = z_i64tostr(va_arg(ap, int64_t), buf, avail, base);
                     else
                         n = z_u64tostr(va_arg(ap, uint64_t), buf, avail, base);
                     break;
                }
                nchars += n;
                frmt += 2;
                buf += n;
                break;
            case 'c':
                *buf++ = (char)va_arg(ap, int);
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

