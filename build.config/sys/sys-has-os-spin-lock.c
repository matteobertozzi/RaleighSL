#include <libkern/OSAtomic.h>

int main (int argc, char **buffer) {
    OSSpinLock lock;

    OSSpinLockLock(&lock);
    OSSpinLockUnlock(&lock);

    return(0);
}
