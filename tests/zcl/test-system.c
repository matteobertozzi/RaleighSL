#include <zcl/system.h>
#include <stdio.h>

#define _2MIB(x)            (((float)(x)) / (1 << 20))

int main (int argc, char **argv) {
    printf(" [ ok ] System information\n");
    printf("        - Processors    %u\n", z_system_processors());
    printf("        - Cache Line    %u\n", z_system_cache_line());
    printf("        - Memory        %4.2fMiB\n", _2MIB(z_system_memory()));
    printf("        - Memory Free   %4.2fMiB\n", _2MIB(z_system_memory_free()));
    printf("        - Memory Used   %4.2fMiB\n", _2MIB(z_system_memory_used()));
    return(0);
}

