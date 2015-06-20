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

#include <sys/time.h>
#include <stdio.h>
#include <time.h>

#include <zcl/time.h>
#include <zcl/math.h>

#if defined(Z_SYS_HAS_MACH_CLOCK_GET_TIME)
  #include <mach/clock.h>
  #include <mach/mach.h>
#endif

#define __NSEC_PER_SEC         ((uint64_t)1000000000U)
#define __NSEC_PER_USEC        ((uint64_t)1000U)
#define __MSEC_PER_USEC        ((uint64_t)1000U)
#define __USEC_PER_SEC         ((uint64_t)1000000U)
#define __MSEC_PER_SEC         ((uint64_t)1000U)

uint32_t z_time_secs (void) {
  struct timeval now;
  gettimeofday(&now, NULL);
  return(now.tv_sec);
}

uint64_t z_time_millis (void) {
  struct timeval now;
  gettimeofday(&now, NULL);
  return(now.tv_sec * __MSEC_PER_SEC + (now.tv_usec / __MSEC_PER_SEC));
}

uint64_t z_time_micros (void) {
#if 0
  uint64_t nanos = z_time_nanos();
  return(z_fast_u32_div1000(nanos));
#else
  struct timeval now;
  gettimeofday(&now, NULL);
  return(now.tv_sec * __USEC_PER_SEC + now.tv_usec);
#endif
}

uint64_t z_time_nanos (void) {
#if defined(Z_SYS_HAS_CLOCK_GETTIME)
  struct timespec now;
  #if defined(CLOCK_MONOTONIC)
    clock_gettime(CLOCK_MONOTONIC, &now);
  #elif defined(CLOCK_MONOTONIC_COARSE)
    clock_gettime(CLOCK_MONOTONIC_COARSE, &now);
  #elif defined(CLOCK_UPTIME)
    clock_gettime(CLOCK_UPTIME, &now);
  #else
    #error "clock_gettime() has no clocks UPTIME/MONOTONIC"
  #endif
  return(now.tv_sec * __NSEC_PER_SEC + now.tv_nsec);
#elif defined(Z_SYS_HAS_MACH_CLOCK_GET_TIME)
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  return(mts.tv_sec * __NSEC_PER_SEC + mts.tv_nsec);
#else
  struct timeval now;
  gettimeofday(&now, NULL);
  return(now.tv_sec * __NSEC_PER_SEC + (now.tv_usec * __NSEC_PER_USEC));
#endif
}

void z_time_usleep (uint64_t usec) {
  struct timespec ts;
  ts.tv_sec = usec / __USEC_PER_SEC;
  ts.tv_nsec = (usec % __USEC_PER_SEC) * 1000;
  nanosleep(&ts, NULL);
}
