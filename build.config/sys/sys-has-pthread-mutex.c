#include <stdlib.h>
#include <pthread.h>

int main (int argc, char **argv) {
    pthread_mutex_t mutex;
    int has_lock;

    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_lock(&mutex);
    has_lock = !pthread_mutex_trylock(&mutex);
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);

    return(0);
}
