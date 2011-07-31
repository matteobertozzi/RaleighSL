#include <stdlib.h>
#include <pthread.h>

int main (int argc, char **argv) {
    pthread_mutex_t mutex;
    pthread_cond_t cond;

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    pthread_mutex_lock(&mutex);
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);

    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);

    return(0);
}

