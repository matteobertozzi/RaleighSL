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

#include <zcl/crypto.h>

static void __test_crypto (z_utest_env_t *env, const z_crypto_vtable_t *vtable) {
  z_cryptor_t cryptor;
  uint8_t input[640];
  uint8_t cbuf[1024];
  uint8_t ubuf[1024];
  uint32_t outlen;
  uint32_t inlen;

  memset(input +   0, 10, 128);
  memset(input + 128, 20, 128);
  memset(input + 256, 30, 128);
  memset(input + 384, 40, 128);
  memset(input + 512, 50, 128);
  memset(cbuf, 1, 1024);
  memset(ubuf, 2, 1024);

  vtable->load(&cryptor);

  vtable->setkey(&cryptor, 107591, "key", 3, "salt", 4);

  for (inlen = 1; inlen < sizeof(input); ++inlen) {
    uint32_t csize;
    int r;

    outlen = vtable->outlen(&cryptor, inlen);
    fprintf(stderr, "inlen %u outlen %u\n", inlen, outlen);

    /* run 1 */
    csize = 1024;
    r = vtable->encrypt(&cryptor, input, inlen, cbuf, &csize);
    z_assert(env, r == 0, "encrypt failed");
    z_assert_u32_equals(env, outlen, csize);
    z_human_dblock_print(stderr, cbuf, csize);
    fprintf(stderr, "\n");

    r = vtable->decrypt(&cryptor, cbuf, csize, ubuf, inlen);
    z_assert(env, r == 0, "decrypt failed");
    z_assert_mem_equals(env, ubuf, inlen, input, inlen);

    /* run 2 */
    csize = 1024;
    r = vtable->encrypt(&cryptor, input, inlen, cbuf, &csize);
    z_assert(env, r == 0, "encrypt failed");
    z_assert_u32_equals(env, outlen, csize);
    z_human_dblock_print(stderr, cbuf, csize);
    fprintf(stderr, "\n");

    r = vtable->decrypt(&cryptor, cbuf, csize, ubuf, inlen);
    z_assert(env, r == 0, "decrypt failed");
    z_assert_mem_equals(env, ubuf, inlen, input, inlen);
  }

  vtable->unload(&cryptor);
}

static void test_crypto_aes (z_utest_env_t *env) {
  __test_crypto(env, &z_crypto_aes);
}

int main (int argc, char **argv) {
  z_utest_run(test_crypto_aes, 0);
  return(0);
}
