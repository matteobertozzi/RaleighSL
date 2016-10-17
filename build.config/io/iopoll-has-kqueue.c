#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>

int main (int argc, char **argv) {
    int kfd;

    if ((kfd = kqueue()) < 0)
        close(kfd);

    return(0);
}
