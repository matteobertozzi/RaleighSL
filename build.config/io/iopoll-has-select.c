#include <sys/select.h>
#include <stdio.h>

int main (int argc, char **argv) {
    struct timeval timeout;
    timeout.tv_usec = 20;
    timeout.tv_sec = 0;
    select(0, NULL, NULL, NULL, &timeout);
    return(0);
}

