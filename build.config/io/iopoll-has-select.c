#include <sys/select.h>
#include <stdlib.h>
#include <stdio.h>

int main (int argc, char **argv) {
  struct timeval tv;
  fd_set rfds;
  fd_set wfds;
  FD_ZERO(&rfds);
  FD_ZERO(&wfds);
  FD_SET(0, &rfds);
  FD_SET(0, &wfds);
  tv.tv_sec  = 5;
  tv.tv_usec = 0;
  if (select(1, &rfds, &wfds, NULL, &tv) <= 0)
    return(1);
  return(!(!FD_ISSET(0, &rfds) && FD_ISSET(0, &wfds)));
}
