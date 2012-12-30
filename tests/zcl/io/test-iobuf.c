/*
 *   Copyright 2011-2013 Matteo Bertozzi
 *
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

#include <zcl/string.h>
#include <zcl/iobuf.h>
#include <zcl/test.h>

static int __test_read_write (void *self) {
  unsigned char *data;
  z_memory_t memory;
  z_iobuf_t iobuf;
  long n;

  z_system_memory_init(&memory);
  z_iobuf_alloc(&iobuf, &memory, 16);

  n = z_iobuf_write(&iobuf, &data);
  z_test_equals(16, n);
  z_test_equals(0, iobuf.offset);
  z_test_equals(16, iobuf.length);
  z_memcpy(data, "01234567", 8);

  z_iobuf_write_backup(&iobuf, 8);
  z_test_equals(0, iobuf.offset);
  z_test_equals(8, iobuf.length);

  n = z_iobuf_read(&iobuf, &data);
  z_test_equals(8, n);
  z_test_equals(8, iobuf.offset);
  z_test_equals(0, iobuf.length);
  z_test_equals(0, z_memcmp(data, "01234567", 8));

  z_iobuf_read_backup(&iobuf, 6);
  z_test_equals(2, iobuf.offset);
  z_test_equals(6, iobuf.length);

  n = z_iobuf_write(&iobuf, &data);
  z_test_equals(8, n);
  z_test_equals(2, iobuf.offset);
  z_test_equals(14, iobuf.length);

  n = z_iobuf_write(&iobuf, &data);
  z_test_equals(16, n);
  z_test_equals(2, iobuf.offset);
  z_test_equals(30, iobuf.length);

  n = z_iobuf_read(&iobuf, &data);
  z_test_equals(14, n);
  z_test_equals(16, iobuf.offset);
  z_test_equals(16, iobuf.length);
  z_test_equals(0, z_memcmp(data, "234567", 6));

  z_iobuf_clear(&iobuf);
  z_test_equals(0, iobuf.offset);
  z_test_equals(0, iobuf.length);
  z_test_equals(NULL, iobuf.head);
  z_test_equals(NULL, iobuf.tail);

  z_iobuf_free(&iobuf);
  return(0);
}

static const z_test_t __test_iobuf = {
    .name       = "Test IOBuf",
    .setup      = NULL,
    .tear_down  = NULL,
    .funcs      = {
        __test_read_write,
        NULL,
    },
};

int main (int argc, char **argv) {
    printf(" [INFO] z_iobuf_t: %lu\n", sizeof(z_iobuf_t));
    return(z_test_run(&__test_iobuf, NULL));
}
