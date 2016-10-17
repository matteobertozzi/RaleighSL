#include <os/lock.h>

int main (int argc, char **buffer) {
    os_unfair_lock_t lock = &OS_UNFAIR_LOCK_INIT;
    int has_lock;

    os_unfair_lock_lock(lock);
    has_lock = os_unfair_lock_trylock(lock);
    os_unfair_lock_unlock(lock);

    return(0);
}
