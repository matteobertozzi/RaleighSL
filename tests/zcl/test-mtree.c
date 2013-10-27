/*
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include <zcl/global.h>
#include <zcl/string.h>
#include <zcl/btree.h>
#include <zcl/debug.h>
#if 0
static void __test_mtree (void) {
  z_btree_t btree;
  int i;

  z_btree_open(&btree);

  z_btree_insert(&btree, z_bytes_from_str("k01"), NULL);
  z_btree_insert(&btree, z_bytes_from_str("k00"), NULL);
  z_btree_insert(&btree, z_bytes_from_str("k03"), NULL);
  z_btree_insert(&btree, z_bytes_from_str("k02"), NULL);

  for (i = 4; i < 20; i += 2) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "k%02d", i);
    fprintf(stderr, "Insert %s\n", buffer);
    z_btree_insert(&btree, z_bytes_from_str(buffer), NULL);
  }

  for (i = 5; i < 20; i += 2) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "k%02d", i);
    fprintf(stderr, "Insert %s\n", buffer);
    z_btree_insert(&btree, z_bytes_from_str(buffer), NULL);
  }

  for (i = 4; i < 20; i += 2) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "k%02d", i);
    fprintf(stderr, "Remove %s\n", buffer);
    z_btree_remove(&btree, z_bytes_from_str(buffer));
  }

  for (i = 5; i < 20; i += 2) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "k%02d", i);
    fprintf(stderr, "Remove %s\n", buffer);
    z_btree_remove(&btree, z_bytes_from_str(buffer));
  }

  z_btree_close(&btree);
}

int main (int argc, char **argv) {
  z_allocator_t allocator;

  /* Initialize allocator */
  if (z_system_allocator_open(&allocator))
    return(1);

  /* Initialize global context */
  if (z_global_context_open(&allocator, NULL)) {
    z_allocator_close(&allocator);
    return(1);
  }

  __test_mtree();

  z_global_context_close();
  z_allocator_close(&allocator);
  return(0);
}
#else
int main (int argc, char **argv) {
  return(0);
}
#endif