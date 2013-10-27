#include <sys/types.h>
#include <sys/epoll.h>

#define _EV_SIZE 4

int main (int argc, char **argv) {
    struct epoll_event ev[_EV_SIZE];
    int nfds;
    int efd;

    efd = epoll_create(_EV_SIZE);

    ev[0].events = EPOLLIN | EPOLLOUT;
    ev[0].data.fd = 0;
    epoll_ctl(efd, EPOLL_CTL_ADD, 0, &(ev[0]));

    if ((nfds = epoll_wait(efd, ev, _EV_SIZE, 20))) {
    }

    return(0);
}
