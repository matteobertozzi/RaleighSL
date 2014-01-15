#include <raleighsl/types.h>

#include <stdio.h>

#define __print_size(type)                                \
  printf("%25s %4ld\n", #type, z_sizeof(type));

int main (int argc, char **argv) {
  printf("RaleighSL Objects\n");
  printf("---------------------------------\n");
  __print_size(raleighsl_t);
  __print_size(raleighsl_key_t);
  __print_size(raleighsl_block_t);
  __print_size(raleighsl_object_t);
  __print_size(raleighsl_transaction_t);
  __print_size(raleighsl_semantic_t);
  __print_size(raleighsl_device_t);
  __print_size(raleighsl_master_t);

  return(0);
}
