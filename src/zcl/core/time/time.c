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

#include <zcl/time.h>

#include <sys/time.h>
#include <time.h>

#if defined(__APPLE__)
  #include <mach/clock.h>
  #include <mach/mach.h>
 #include <mach/mach_time.h>
#endif

#define __NSEC_PER_SEC         ((uint64_t)1000000000ul)
#define __NSEC_PER_USEC        ((uint64_t)1000ul)
#define __MSEC_PER_USEC        ((uint64_t)1000ul)
#define __USEC_PER_SEC         ((uint64_t)1000000ul)
#define __MSEC_PER_SEC         ((uint64_t)1000ul)

uint64_t z_time_millis (void) {
  struct timeval now;
  gettimeofday(&now, NULL);
  return(now.tv_sec * __MSEC_PER_SEC + (now.tv_usec / __MSEC_PER_SEC));
}

uint64_t z_time_micros (void) {
  struct timeval now;
  gettimeofday(&now, NULL);
  return(now.tv_sec * __USEC_PER_SEC + now.tv_usec);
}

uint64_t z_time_monotonic (void) {
#if defined(__APPLE__)
  #if 1
    static double __scaling_factor = 0;
    if (Z_UNLIKELY(__scaling_factor == 0)) {
      mach_timebase_info_data_t info;
      mach_timebase_info(&info);
      __scaling_factor = (double)info.numer / (double)info.denom;
    }
    return mach_absolute_time() * __scaling_factor;
  #else
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    return(mts.tv_sec * __NSEC_PER_SEC + mts.tv_nsec);
  #endif
#else
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return(now.tv_sec * __NSEC_PER_SEC + now.tv_nsec);
#endif
}

void z_time_usleep (uint64_t usec) {
  struct timespec ts;
  ts.tv_sec = usec / 1000000U;
  ts.tv_nsec = (usec % 1000000U) * 1000;
  nanosleep(&ts, NULL);
}
