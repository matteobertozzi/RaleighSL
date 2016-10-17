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

#include <zcl/waitcond.h>
#include <zcl/utest.h>
#include <zcl/mutex.h>
#include <zcl/shmem.h>
#include <zcl/time.h>

#include <unistd.h>

struct test_shmem {
  z_shmem_t shmem;
  z_mutex_t *mutex;
  z_wait_cond_t *wcond;
  long *shdata;
};

static void __test_shmem_init (struct test_shmem *self) {
  uint8_t *p = self->shmem.addr;
  self->mutex = Z_CAST(z_mutex_t, p); p += sizeof(z_mutex_t);
  self->wcond = Z_CAST(z_wait_cond_t, p); p+= sizeof(z_wait_cond_t);
  self->shdata = Z_CAST(long, p);
}

static int __test_shmem_create (struct test_shmem *self, const char *path, int size) {
  if (z_shmem_create(&(self->shmem), path, size))
    return -1;

  __test_shmem_init(self);

  if (z_mutex_shmem_init(self->mutex)) {
    z_shmem_close(&(self->shmem), 1);
    return -2;
  }

  if (z_wait_cond_shmem_init(self->wcond)) {
    z_mutex_destroy(self->mutex);
    z_shmem_close(&(self->shmem), 1);
    return -3;
  }

  return(0);
}

static int __test_shmem_open (struct test_shmem *self, const char *path, int rdonly) {
  if (z_shmem_open(&(self->shmem), path, rdonly))
    return -1;

  __test_shmem_init(self);
  return(0);
}

static int __test_shmem_close (struct test_shmem *self, int destroy) {
  if (destroy) {
    z_wait_cond_destroy(self->wcond);
    z_mutex_destroy(self->mutex);
  }
  return z_shmem_close(&(self->shmem), destroy);
}

static void test_shmem (z_utest_env_t *env) {
  const char *path = "/zcl-test.shmem";
  struct test_shmem shmem;
  int r;

  r = __test_shmem_create(&shmem, path, 4096);
  z_assert_false(env, r < 0);

  *(shmem.shdata) = 5;

  r = __test_shmem_close(&shmem, 0);
  z_assert_false(env, r < 0);

  if (!fork()) {
    int spin = 1;

    r = __test_shmem_open(&shmem, path, 0);
    z_assert_false(env, r < 0);

    while (spin) {
      z_mutex_lock(shmem.mutex);
      spin = (*(shmem.shdata) != 10);
      z_mutex_unlock(shmem.mutex);
    }

    *(shmem.shdata) = 20;
    z_mutex_lock(shmem.mutex);
    z_wait_cond_broadcast(shmem.wcond);
    z_mutex_unlock(shmem.mutex);

    r = __test_shmem_close(&shmem, 0);
    z_assert_false(env, r < 0);
    exit(0);
  }

  r = __test_shmem_open(&shmem, path, 0);
  z_assert_false(env, r < 0);

  z_mutex_lock(shmem.mutex);
  *(shmem.shdata) = 10;
  z_wait_cond_wait(shmem.wcond, shmem.mutex, Z_TIME_SECONDS_TO_MICROS(10));
  z_mutex_unlock(shmem.mutex);
  z_assert_long_equals(env, 20, *(shmem.shdata));

  r = __test_shmem_close(&shmem, 1);
  z_assert_false(env, r < 0);
}

int main (int argc, char **argv) {
  int r = 0;
  r |= z_utest_run(test_shmem, 60000);
  return(r);
}
