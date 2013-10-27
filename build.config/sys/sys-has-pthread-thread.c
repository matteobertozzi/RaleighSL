#include <stdlib.h>
#include <pthread.h>

static void *__thread_func (void *arg) {
    return(arg);
}

int main (int argc, char **argv) {
    pthread_t thread;

    pthread_create(&thread, NULL, __thread_func, NULL);
    pthread_join(thread, NULL);
    pthread_self();

    return(0);
}

