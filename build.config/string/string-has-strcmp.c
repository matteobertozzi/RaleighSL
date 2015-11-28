#include <string.h>

int main (int argc, char **buffer) {
    char buf[10] = "hello";
    int cmp;
    if ((cmp = strcmp(buf, "hello")))
        return(1);
    return(0);
}

