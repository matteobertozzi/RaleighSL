#include <poll.h>

int main (int argc, char **argv) {
  struct pollfd ev;
  ev.fd = 0;
  ev.events  = POLLOUT | POLLIN | POLLERR | POLLHUP;
  ev.revents = 0;
  if (poll(&ev, 1, 1000) <= 0)
    return(-1);
  return(0);
}
