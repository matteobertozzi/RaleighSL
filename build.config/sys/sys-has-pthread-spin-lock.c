#include <pthread.h>

int main (int argc, char **buffer) {
    pthread_spinlock_t lock;
    int has_lock;

    pthread_spin_init(&lock, 0);
    pthread_spin_lock(&lock);
    has_lock = !pthread_spin_trylock(&lock);
    pthread_spin_unlock(&lock);
    pthread_spin_destroy(&lock);

    return(0);
}
