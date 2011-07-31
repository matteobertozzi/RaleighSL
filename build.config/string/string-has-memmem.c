#include <string.h>

int main (int argc, char **buffer) {
    char buf[10] = "01Hello78";
    void *ptr;

    if ((ptr = memmem(buf, 10, "Hello", 5)))
        return(1);

    return(0);
}

