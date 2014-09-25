#include <string.h>

int main (int argc, char **buffer) {
    char buf[10] = "hello";
    int cmp;
    if ((cmp = strncmp(buf, "hello", 5)))
        return(1);
    return(0);
}

