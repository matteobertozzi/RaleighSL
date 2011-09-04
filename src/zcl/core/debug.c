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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <zcl/debug.h>

static const char __weekday_name[7][3] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static const char __month_name[12][3] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static void __date_time (char buffer[25]) {
    struct tm datetime;
    time_t t;

    t = time(NULL);
    localtime_r(&t, &datetime);

    snprintf(buffer, 25, "%.3s %.3s %2d %d %.2d:%.2d:%.2d ",
             __weekday_name[datetime.tm_wday],
             __month_name[datetime.tm_mon],
             datetime.tm_mday, 1900 + datetime.tm_year,
             datetime.tm_hour, datetime.tm_min, datetime.tm_sec);
}

void __z_log (FILE *fp,
              const char *file, int line, const char *func,
              const char *format, ...)
{
    char datetime[25];
    va_list ap;

    __date_time(datetime);
    fprintf(stderr, "[%s] %s:%d %s - ",
            datetime, file, line, func);

    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
}

void __z_bug (const char *file, int line, const char *func,
              const char *format, ...)
{
    char datetime[25];
    va_list ap;

    __date_time(datetime);
    fprintf(stderr, "[%s] BUG at %s:%d in function %s(). ",
            datetime, file, line, func);

    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);

    abort();
}

void __z_assert (const char *file, int line, const char *func,
                 int vcond, const char *condition,
                 const char *format, ...)
{
    char datetime[25];
    va_list ap;

    if (vcond)
        return;

    __date_time(datetime);
    fprintf(stderr, "[%s] ASSERTION (%s) at %s:%d in function %s() failed. ",
            datetime, condition, file, line, func);

    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);

    abort();
}

