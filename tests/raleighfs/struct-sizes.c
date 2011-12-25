#include <raleighfs/raleighfs.h>

#include <stdio.h>

#define __usizeof(x)                ((unsigned int)sizeof(x))

int main (int argc, char **argv) {
    printf("raleighfs_t          %u\n", __usizeof(raleighfs_t));
    printf("raleighfs_key_t      %u\n", __usizeof(raleighfs_key_t));
    printf("raleighfs_rwlock_t   %u\n", __usizeof(raleighfs_rwlock_t));
    printf("raleighfs_device_t   %u\n", __usizeof(raleighfs_device_t));
    printf("raleighfs_master_t   %u\n", __usizeof(raleighfs_master_t));
    printf("raleighfs_object_t   %u\n", __usizeof(raleighfs_object_t));
    printf("raleighfs_objdata_t  %u\n", __usizeof(raleighfs_objdata_t));
    return(0);
}

