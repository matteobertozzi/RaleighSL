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
#include <zcl/task.h>
#include <zcl/math.h>
#include <zcl/mem.h>

#include <signal.h>
#include <unistd.h>
#include <stdio.h>

struct task_data {
  z_task_t vtask;
  int count;
};

static z_task_rstate_t __task_func0 (z_task_t *task) {
  struct task_data *data = z_container_of(task, struct task_data, vtask);
  printf("[S] Exec Task %"PRIu64" Count %d\n", data->vtask.vtask.seqid, data->count);
  usleep(200 * 1000);
  printf("[E] Exec Task %"PRIu64" Count %d\n", data->vtask.vtask.seqid, data->count);
  if (++data->count < 2) {
    z_vtask_resume(&(task->vtask));
    return(Z_TASK_CONTINUATION);
  }
  return(Z_TASK_COMPLETED);
}

static z_task_rstate_t __task_barrier (z_task_t *task) {
  printf("[-] Exec Task Barrier %"PRIu64"\n", task->vtask.seqid);
  usleep(500 * 1000);
  printf("[-] Exec Task Barrier %"PRIu64"\n", task->vtask.seqid);
  return(Z_TASK_COMPLETED);
}

int main (int argc, char **argv) {
  struct task_data data[6];
  z_allocator_t allocator;
  z_task_rq_group_t group;

  z_memzero(data, sizeof(data));

  /* Initialize allocator */
  if (z_system_allocator_open(&allocator))
    return(1);

  /* Initialize global context */
  if (z_global_context_open(&allocator, NULL)) {
    z_allocator_close(&allocator);
    return(1);
  }

  if (argc > 1) {
    z_task_group_open(&group, 0);

    z_task_reset(&(data[0].vtask), __task_func0);
    z_task_group_add(&group, &(data[0].vtask.vtask));

    z_task_reset(&(data[1].vtask), __task_func0);
    z_task_group_add(&group, &(data[1].vtask.vtask));

    z_task_reset(&(data[2].vtask), __task_barrier);
    z_task_group_add_barrier(&group, &(data[2].vtask.vtask));

    z_task_reset(&(data[3].vtask), __task_func0);
    z_task_group_add(&group, &(data[3].vtask.vtask));

    z_task_reset(&(data[4].vtask), __task_func0);
    z_task_group_add(&group, &(data[4].vtask.vtask));

    z_task_reset(&(data[5].vtask), __task_barrier);
    z_task_group_add_barrier(&group, &(data[5].vtask.vtask));

    z_task_rq_add(z_global_rq(), &(group.rq.vtask));

    z_task_group_wait(&group);
    z_task_group_close(&group);
  }

  z_global_context_close();
  z_allocator_close(&allocator);
  return(0);
}
