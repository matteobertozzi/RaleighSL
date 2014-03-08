#include <stdlib.h>
#include <pthread.h>

int main (int argc, char **argv) {
    pthread_rwlock_t lock;

    pthread_rwlock_init(&lock, NULL);
    pthread_rwlock_rdlock(&lock);
    pthread_rwlock_unlock(&lock);
    pthread_rwlock_wrlock(&lock);
    pthread_rwlock_unlock(&lock);
    pthread_rwlock_destroy(&lock);

    return(0);
}

