#include <string.h>

int main (int argc, char **buffer) {
    char buf[10];
    memmove(buf, buf + 4, 4);
    return(0);
}

