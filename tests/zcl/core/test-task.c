/*
 *   Copyright 2007-2013 Matteo Bertozzi
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

#include <zcl/global.h>
#include <zcl/debug.h>
#include <zcl/math.h>

#include <signal.h>
#include <unistd.h>
#include <stdio.h>

#define NUM_TASKS     16

struct task_data {
  z_task_t vtask;
  int task_id;
  unsigned int seed;
  int count;
};

static void __task_func (z_task_t *task) {
  struct task_data *data = z_container_of(task, struct task_data, vtask);
  //printf("Exec Task %d Count %d\n", data->task_id, data->count);
  usleep(20 * 1000);
  if (data->count++ < 100) {
    z_global_add_task(task);
  }
}

int main (int argc, char **argv) {
  struct task_data data[NUM_TASKS];
  z_allocator_t allocator;
  unsigned int seed = 1;
  int i;

  /* Initialize allocator */
  if (z_system_allocator_open(&allocator))
    return(1);

  /* Initialize global context */
  if (z_global_context_open(&allocator, NULL)) {
    z_allocator_close(&allocator);
    return(1);
  }

  for (i = 0; i < NUM_TASKS; ++i) {
    z_task_reset(&(data[i].vtask), __task_func);
    data[i].task_id = i;
    data[i].seed = z_rand(&seed);
    data[i].count = 0;
    z_global_add_task(&(data[i].vtask));
  }

  for (i = 0; i < ((argc - 1) * 100); ++i) {
    fprintf(stderr, "sleep %d\n", i);
    int j, k;
    for (j = 0; j < NUM_TASKS; ++j) {
      fprintf(stderr, "\n%2d: ", j);
      for (k = 0; k < data[j].count; ++k) {
        fprintf(stderr, "=");
      }
    }
    fprintf(stderr, "\n");
    usleep(200 * 1000);
  }

  z_global_context_close();
  z_allocator_close(&allocator);
  return(0);
}
