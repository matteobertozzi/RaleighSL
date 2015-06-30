/*
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

#include <zcl/macros.h>

#include <zcl/humans.h>
#include <zcl/time.h>

#include <errno.h>
#include <stdio.h>

#define Z_LOG_LEVEL_CODER            0
#define Z_LOG_LEVEL_FATAL            1
#define Z_LOG_LEVEL_ERROR            2
#define Z_LOG_LEVEL_WARN             3
#define Z_LOG_LEVEL_INFO             4
#define Z_LOG_LEVEL_DEBUG            5
#define Z_LOG_LEVEL_TRACE            6
#define Z_LOG_LEVEL_DEFAULT          Z_LOG_LEVEL_TRACE

#define z_log_is_level_enabled(exp_level)                     \
  (z_debug_get_log_level() >= (exp_level))

#define z_log_is_coder_enabled()      z_log_is_level_enabled(Z_LOG_LEVEL_DEBUG)
#define z_log_is_fatal_enabled()      z_log_is_level_enabled(Z_LOG_LEVEL_FATAL)
#define z_log_is_error_enabled()      z_log_is_level_enabled(Z_LOG_LEVEL_ERROR)
#define z_log_is_warn_enabled()       z_log_is_level_enabled(Z_LOG_LEVEL_WARN)
#define z_log_is_info_enabled()       z_log_is_level_enabled(Z_LOG_LEVEL_INFO)
#define z_log_is_debug_enabled()      z_log_is_level_enabled(Z_LOG_LEVEL_DEBUG)
#define z_log_is_trace_enabled()      z_log_is_level_enabled(Z_LOG_LEVEL_TRACE)

#define Z_DEBUG(frmt, ...)         Z_LOG(Z_LOG_LEVEL_CODER, frmt, ##__VA_ARGS__)

#define Z_LOG_FATAL(frmt, ...)     Z_LOG(Z_LOG_LEVEL_FATAL, frmt, ##__VA_ARGS__)
#define Z_LOG_ERROR(frmt, ...)     Z_LOG(Z_LOG_LEVEL_ERROR, frmt, ##__VA_ARGS__)
#define Z_LOG_WARN(frmt, ...)      Z_LOG(Z_LOG_LEVEL_WARN,  frmt, ##__VA_ARGS__)
#define Z_LOG_INFO(frmt, ...)      Z_LOG(Z_LOG_LEVEL_INFO,  frmt, ##__VA_ARGS__)
#define Z_LOG_DEBUG(frmt, ...)     Z_LOG(Z_LOG_LEVEL_DEBUG, frmt, ##__VA_ARGS__)
#define Z_LOG_TRACE(frmt, ...)     Z_LOG(Z_LOG_LEVEL_TRACE, frmt, ##__VA_ARGS__)

#define Z_LOG_ERRNO_FATAL(frmt, ...)  Z_LOG_ERRNO(Z_LOG_LEVEL_FATAL, frmt, ##__VA_ARGS__)
#define Z_LOG_ERRNO_ERROR(frmt, ...)  Z_LOG_ERRNO(Z_LOG_LEVEL_ERROR, frmt, ##__VA_ARGS__)
#define Z_LOG_ERRNO_WARN(frmt, ...)   Z_LOG_ERRNO(Z_LOG_LEVEL_WARN,  frmt, ##__VA_ARGS__)
#define Z_LOG_ERRNO_INFO(frmt, ...)   Z_LOG_ERRNO(Z_LOG_LEVEL_INFO,  frmt, ##__VA_ARGS__)
#define Z_LOG_ERRNO_DEBUG(frmt, ...)  Z_LOG_ERRNO(Z_LOG_LEVEL_DEBUG, frmt, ##__VA_ARGS__)
#define Z_LOG_ERRNO_TRACE(frmt, ...)  Z_LOG_ERRNO(Z_LOG_LEVEL_TRACE, frmt, ##__VA_ARGS__)

#define Z_LOG(level, format, ...)                                       \
  Z_FLOG(stderr, level, format, ##__VA_ARGS__)

#define Z_FLOG(fp, level, format, ...)                                  \
  __Z_FLOG(fp, level, 0, format, ##__VA_ARGS__)

#define Z_LOG_ERRNO(level, format, ...)                                 \
  Z_FLOG_ERRNO(stderr, level, format, ##__VA_ARGS__)

#define Z_FLOG_ERRNO(fp, level, format, ...)                            \
  __Z_FLOG(stderr, level, errno, format, ##__VA_ARGS__)

#define THZ_FORCE_LOG       1
#define THZ_FORCE_ASSERT    1

#if THZ_FORCE_LOG
  #define __Z_FLOG(fp, level, errnum, format, ...)                       \
    if (level <= z_debug_get_log_level())                                \
      __z_log(fp, level, errnum, __FILE__, __LINE__, __FUNCTION__,       \
              format, ##__VA_ARGS__)
#else
  #define __Z_FLOG(fp, level, errnum, format, ...)          while (0)
#endif

#if THZ_FORCE_ASSERT && __Z_DEBUG__
  #define Z_ASSERT(cond, format, ...)                                     \
    if (Z_UNLIKELY(!(cond)))                                              \
      __z_assert(__FILE__, __LINE__, __FUNCTION__,                        \
                 cond, #cond, format, ##__VA_ARGS__)

  #define Z_ASSERT_IF(ifcond, cond, format, ...)                          \
    if (Z_UNLIKELY(ifcond)) Z_ASSERT(cond, format, ##__VA_ARGS__)

  #define Z_PRINT_DEBUG(format, ...)                                      \
    fprintf(stderr, format, ##__VA_ARGS__)
#else
  #define Z_ASSERT(cond, format, ...)             while (0) { (void)(cond); }
  #define Z_ASSERT_IF(ifcond, cond, format, ...)  while (0) { (void)(cond); }
  #define Z_PRINT_DEBUG(format, ...)              while (0)
#endif

#define Z_ASSERT_BUG(cond, format, ...)                                   \
  if (Z_UNLIKELY(!(cond)))                                                \
      __z_assert(__FILE__, __LINE__, __FUNCTION__,                        \
                 cond, #cond, format, ##__VA_ARGS__)

void __z_log    (FILE *fp, int level, int errnum,
                 const char *file, int line, const char *func,
                 const char *format, ...);
void __z_assert (const char *file, int line, const char *func,
                 int vcond, const char *condition,
                 const char *format, ...);

int  z_stack_trace      (char *buffer, size_t bufsize, int levels);
void z_dump_stack_trace (FILE *fp, int levels);

void z_debug_open  (void);
void z_debug_close (void);
int  z_debug_get_log_level (void);
void z_debug_set_log_level (int level);

__Z_END_DECLS__

#endif /* !_Z_DEBUG_H_ */
