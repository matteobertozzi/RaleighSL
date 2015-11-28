#include <pthread.h>

int main (int argc, char **argv) {
    pthread_yield_np();
    return(0);
}

