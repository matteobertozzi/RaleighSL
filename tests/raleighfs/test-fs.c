#include <raleighfs/semantic/flat.h>
#include <raleighfs/device/memory.h>
#include <raleighfs/key/flat.h>

#include <raleighfs/raleighfs.h>

#include <zcl/memory.h>

#include <stdio.h>

int main (int argc, char **argv) {
    raleighfs_device_t device;
    raleighfs_errno_t errno;
    z_memory_t memory;
    raleighfs_t fs;

    z_memory_init(&memory, z_system_allocator());
    if (raleighfs_alloc(&fs, &memory) == NULL)
        return(1);

    /* Plug semantic layer */
    raleighfs_plug_semantic(&fs, &raleighfs_semantic_flat);

    /* Plug objects */

    raleighfs_memory_device(&device);
    if ((errno = raleighfs_create(&fs, &device,
                                  &raleighfs_key_flat,
                                  NULL, NULL,
                                  &raleighfs_semantic_flat)))
    {
        fprintf(stderr, "raleighfs_create(): %s\n",
                raleighfs_errno_string(errno));
        return(2);
    }


    raleighfs_close(&fs);
    raleighfs_free(&fs);
    return(0);
}

