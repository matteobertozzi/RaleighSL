#include <mach/clock.h>
#include <mach/mach.h>
#include <time.h>

int main (int argc, char **argv) {
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  return(0);
}