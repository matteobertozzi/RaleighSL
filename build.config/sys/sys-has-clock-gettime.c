#include <stdio.h>
#include <time.h>

int main (int argc, char **argv) {
  struct timespec now;
#if defined(CLOCK_UPTIME)
  fprintf(stderr, "CLOCK CLOCK UPTIME");
  clock_gettime(CLOCK_UPTIME, &now);
#elif defined(CLOCK_MONOTONIC)
  fprintf(stderr, "CLOCK CLOCK_MONOTONIC");
  clock_gettime(CLOCK_MONOTONIC, &now);
#else
  return(1);
#endif
  return(0);
}