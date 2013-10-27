#include <string.h>

int main (int argc, char **buffer) {
    char buf[10] = "01234A678";
    void *ptr;
    if ((ptr = memrchr(buf, 'A', 10)) == NULL)
        return(1);
    return(0);
}

