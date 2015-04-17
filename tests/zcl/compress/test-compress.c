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

#include <zcl/utest.h>

#include <zcl/compress.h>

static void __test_compress (z_utest_env_t *env,
                             const z_compress_vtable_t *vtable)
{
  z_compressor_t compr;
  uint8_t input[1024];
  uint8_t cbuf[1024];
  uint8_t ubuf[1024];
  uint32_t csize;
  int r;

  memset(input +   0, 10, 128);
  memset(input + 128, 20, 128);
  memset(input + 256, 30, 128);
  memset(input + 384, 40, 128);
  memset(input + 512, 50, 128);
  memset(input + 640, 60, 128);
  memset(input + 768, 70, 128);
  memset(input + 896, 80, 128);
  memset(cbuf, 1, 1024);
  memset(ubuf, 2, 1024);

  compr.level = 10;

  vtable->load(&compr);

  csize = 1024;
  r = vtable->compress(&compr, input, 1024, cbuf, &csize);
  z_assert(env, r == 0, "compression failed");
  z_human_dblock_print(stderr, cbuf, 30);
  fprintf(stderr, "\n");

  r = vtable->decompress(&compr, cbuf, csize, ubuf, 1024);
  z_assert(env, r == 0, "decompression failed");
  z_assert_mem_equals(env, ubuf, 1024, input, 1024);

  vtable->unload(&compr);
}

static void test_compress_lz4 (z_utest_env_t *env) {
  __test_compress(env, &z_compress_lz4);
}

static void test_compress_zlib (z_utest_env_t *env) {
  __test_compress(env, &z_compress_zlib);
}

int main (int argc, char **argv) {
  z_utest_run(test_compress_lz4, 0);
  z_utest_run(test_compress_zlib, 0);
  return(0);
}
