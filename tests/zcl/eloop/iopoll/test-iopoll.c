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
#include <zcl/iopoll.h>
#include <zcl/fd.h>

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

struct buffer {
  char data[512];
  size_t size;
};

struct test_object {
  z_iopoll_engine_t engine;
  z_iopoll_entity_t timer;
  z_iopoll_entity_t uevent;
  z_iopoll_entity_t io_entity;
  uint64_t timer_ev_count;
  uint64_t uevent_ev_count;
  struct buffer rdbuf;
  struct buffer wrbuf;
};

static void buffer_write (struct buffer *self, const char *data, size_t size) {
  memcpy(self->data + self->size, data, size);
  self->size += size;
}

static int __timer_timeout (z_iopoll_engine_t *engine, z_iopoll_entity_t *entity) {
  struct test_object *obj = z_container_of(entity, struct test_object, timer);
  obj->timer_ev_count++;
  z_iopoll_engine_notify(engine, &(obj->uevent));
  return(0);
}

static int __timer_uevent (z_iopoll_engine_t *engine, z_iopoll_entity_t *entity) {
  struct test_object *obj = z_container_of(entity, struct test_object, uevent);
  obj->uevent_ev_count++;
  return(0);
}

static int __io_read (z_iopoll_engine_t *engine, z_iopoll_entity_t *entity) {
  struct test_object *obj = z_container_of(entity, struct test_object, io_entity);
  char buffer[512];
  ssize_t rd;
  rd = read(entity->fd, buffer, sizeof(buffer));
  buffer_write(&(obj->rdbuf), buffer, rd);
  return(0);
}

static int __io_write (z_iopoll_engine_t *engine, z_iopoll_entity_t *entity) {
  write(entity->fd, "ok", 2);
  return(0);
}

const z_iopoll_entity_vtable_t __test_vtable = {
  .read     = __io_read,
  .write    = __io_write,
  .uevent   = __timer_uevent,
  .timeout  = __timer_timeout,
  .close    = NULL,
};

void test_object_open (struct test_object *self) {
  z_memzero(self, sizeof(struct test_object));
  z_iopoll_engine_open(&(self->engine));
  z_iopoll_timer_open(&(self->timer), &__test_vtable, 0);
  z_iopoll_uevent_open(&(self->uevent), &__test_vtable, 0);
  z_iopoll_engine_uevent(&(self->engine), &(self->uevent), 1);
  self->timer_ev_count = 0;
  self->uevent_ev_count = 0;
}

void test_object_close (struct test_object *self) {
  z_iopoll_engine_uevent(&(self->engine), &(self->uevent), 0);
  z_iopoll_engine_close(&(self->engine));
  //z_iopoll_engine_dump(&(self->engine), stdout);
}

static void test_iopoll_events (z_utest_env_t *env) {
  struct test_object obj;
  int i;

  test_object_open(&obj);

  z_iopoll_engine_timer(&obj.engine, &obj.timer, 50);
  for (i = 0; i < 10; ++i) {
    z_iopoll_engine_poll(&obj.engine);
  }
  z_iopoll_engine_timer(&obj.engine, &obj.timer, 0);

  z_assert_u64_equals(env, 5, obj.timer_ev_count);
  z_assert_u64_equals(env, 5, obj.uevent_ev_count);

  test_object_close(&obj);
}

static void test_iopoll_read_write (z_utest_env_t *env) {
  struct test_object obj;
  int fds[2];
  int i;

  test_object_open(&obj);

  pipe(fds);

  z_iopoll_entity_open(&(obj.io_entity), &__test_vtable, fds[0]);
  z_iopoll_engine_add(&obj.engine, &obj.io_entity);
  for (i = 0; i < 10; ++i) {
    char buffer[8];
    int n = snprintf(buffer, 8, "%d ", i);
    write(fds[1], buffer, n);
    buffer_write(&obj.wrbuf, buffer, n);
    z_iopoll_engine_poll(&obj.engine);
  }
  z_assert_true(env, obj.rdbuf.size > 0);
  z_assert_mem_equals(env, obj.wrbuf.data, obj.wrbuf.size, obj.rdbuf.data, obj.rdbuf.size);

  z_iopoll_engine_remove(&obj.engine, &obj.io_entity);
  test_object_close(&obj);

  close(fds[0]);
  close(fds[1]);
}

int main (int argc, char **argv) {
  int r = 0;
  r |= z_utest_run(test_iopoll_events, 60000);
  r |= z_utest_run(test_iopoll_read_write, 60000);
  return(r);
}
