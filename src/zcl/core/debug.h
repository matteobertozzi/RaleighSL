/*
 *   Copyright 2011-2012 Matteo Bertozzi
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

#ifndef _Z_DEBUG_H_
#define _Z_DEBUG_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <stdio.h>

#if __Z_DEBUG__
    #define Z_LOG_FATAL(format, ...)     Z_LOG(0, format, ##__VA_ARGS__)
    #define Z_LOG_ERROR(format, ...)     Z_LOG(1, format, ##__VA_ARGS__)
    #define Z_LOG_WARN(format, ...)      Z_LOG(2, format, ##__VA_ARGS__)
    #define Z_LOG_INFO(format, ...)      Z_LOG(3, format, ##__VA_ARGS__)
    #define Z_LOG_DEBUG(format, ...)     Z_LOG(4, format, ##__VA_ARGS__)
    #define Z_LOG_TRACE(format, ...)     Z_LOG(5, format, ##__VA_ARGS__)

    #define Z_LOG(level, format, ...)                                       \
        Z_FLOG(stderr, level, format, ##__VA_ARGS__)

    #define Z_FLOG(fp, level, format, ...)                                  \
        __z_log(fp, level, __FILE__, __LINE__, __FUNCTION__,                \
                format, ##__VA_ARGS__)

    #define Z_BUG(format, ...)                                              \
        __z_bug(__FILE__, __LINE__, __FUNCTION__,                           \
                format, ##__VA_ARGS__)

    #define Z_BUG_ON(cond, format, ...)                                     \
        if (cond) Z_BUG(format, ##__VA_ARGS__)

    #define Z_ASSERT(cond, format, ...)                                     \
        __z_assert(__FILE__, __LINE__, __FUNCTION__,                        \
                   cond, #cond, format, ##__VA_ARGS__)

    #define Z_ASSERT_IF(ifcond, cond, format, ...)                          \
        if (ifcond) Z_ASSERT(cond, format, ##__VA_ARGS__)

    #define Z_PRINT_DEBUG(format, ...)                                      \
        fprintf(stderr, format, ##__VA_ARGS__)
#else
    #define Z_LOG_FATAL(format, ...)                while (0)
    #define Z_LOG_ERROR(format, ...)                while (0)
    #define Z_LOG_WARN(format, ...)                 while (0)
    #define Z_LOG_INFO(format, ...)                 while (0)
    #define Z_LOG_DEBUG(format, ...)                while (0)
    #define Z_LOG_TRACE(format, ...)                while (0)

    #define Z_LOG(level, format, ...)               while (0)
    #define Z_FLOG(fp, level, format, ...)          while (0)

    #define Z_BUG(format, ...)                      while (0)
    #define Z_ASSERT(cond, format, ...)             while (0)
    #define Z_ASSERT_IF(ifcond, cond, format, ...)  while (0)

    #define Z_PRINT_DEBUG(format, ...)              while (0)
#endif

void __z_log    (FILE *fp, int level,
                 const char *file, int line, const char *func,
                 const char *format, ...);

void __z_bug    (const char *file, int line, const char *func,
                 const char *format, ...);

void __z_assert (const char *file, int line, const char *func,
                 int vcond, const char *condition,
                 const char *format, ...);

void z_dump_stack_trace (FILE *fp, unsigned int levels);

__Z_END_DECLS__

#endif /* !_Z_DEBUG_H_ */

