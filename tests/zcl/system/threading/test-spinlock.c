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
#include <zcl/spinlock.h>

static void test_spinlock (z_utest_env_t *env) {
  z_spinlock_t lock;
  int has_lock;

  z_spin_alloc(&lock);

  has_lock = z_spin_try_lock(&lock);
  z_assert_true(env, has_lock);
  z_spin_unlock(&lock);

  z_spin_lock(&lock);
  has_lock = z_spin_try_lock(&lock);
  z_assert_false(env, has_lock);
  z_spin_unlock(&lock);

  z_spin_free(&lock);
}

int main (int argc, char **argv) {
  int r = 0;
  r |= z_utest_run(test_spinlock, 60000);
  return(r);
}
