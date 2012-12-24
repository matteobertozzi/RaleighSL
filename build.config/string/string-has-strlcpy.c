#include <string.h>

int main (int argc, char **buffer) {
    char abuf[10];
    char bbuf[10];

    strlcpy(abuf, bbuf, 6);

    return(0);
}

