#include <raleighfs/types.h>

#include <stdio.h>

#define __print_size(type)                                \
  printf("%25s %4ld\n", #type, z_sizeof(type));

int main (int argc, char **argv) {
  printf("RaleighFS Objects\n");
  printf("---------------------------------\n");
  __print_size(raleighfs_t);
  __print_size(raleighfs_key_t);
  __print_size(raleighfs_object_t);
  __print_size(raleighfs_transaction_t);
  __print_size(raleighfs_semantic_t);
  __print_size(raleighfs_device_t);
  __print_size(raleighfs_master_t);

  return(0);
}
