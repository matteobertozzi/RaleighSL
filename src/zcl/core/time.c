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

#include <sys/time.h>
#include <stdio.h>

#include <zcl/time.h>

#define __NSEC_PER_SEC         ((uint64_t)1000000000U)
#define __NSEC_PER_USEC        ((uint64_t)1000U)
#define __MSEC_PER_USEC        ((uint64_t)1000U)
#define __USEC_PER_SEC         ((uint64_t)1000000U)
#define __MSEC_PER_SEC         ((uint64_t)1000U)

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

uint64_t z_time_nanos (void) {
#if defined(Z_TIME_HAS_CLOCK_GETTIME)
    struct timespec now;

    #if defined(CLOCK_UPTIME)
        clock_gettime(CLOCK_UPTIME, &now);
    #elif defined(CLOCK_MONOTONIC)
        clock_gettime(CLOCK_MONOTONIC, &now);
    #else
        #warning "clock_gettime() has no clocks UPTIME/MONOTONIC"
    #endif

    return(now.tv_sec * __NSEC_PER_SEC + now.tv_nsec);
#else
    struct timeval now;
    gettimeofday(&now, NULL);
    return(now.tv_sec * __NSEC_PER_SEC + (now.tv_usec * __NSEC_PER_USEC));
#endif
}

void z_timer_start (z_timer_t *timer) {
    timer->start = z_time_micros();
}

void z_timer_stop (z_timer_t *timer) {
    timer->end = z_time_micros();
}

uint64_t z_timer_micros (const z_timer_t *timer) {
    return(timer->end - timer->start);
}

uint64_t z_timer_millis (const z_timer_t *timer) {
    return((timer->end - timer->start) / __MSEC_PER_USEC);
}

float z_timer_secs (const z_timer_t *timer) {
    return((float)(timer->end - timer->start) / __USEC_PER_SEC);
}

