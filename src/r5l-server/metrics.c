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

#include <sys/resource.h>

#include <zcl/debug.h>
#include <zcl/task.h>
#include <zcl/time.h>
#include <zcl/ipc.h>
#include <zcl/fd.h>

#include "server.h"
#include "rpc/generated/metrics_rpc.h"
#include "rpc/generated/metrics.h"

struct metrics_rpc_task {
  z_task_t __task__;
  z_ipc_msg_client_t *client;
  uint64_t msg_id;
  uint32_t msg_type;
  uint16_t request_len;
  uint8_t  request[1];
} __attribute__((packed));

static z_ipc_msg_t *__msg_from_task (z_memory_t *memory,
                                     metrics_rpc_task_t *task,
                                     z_dbuf_writer_t *writer)
{
  z_ipc_msg_head_t msg_head;

  msg_head.msg_type = task->msg_type;
  msg_head.msg_id = task->msg_id;
  msg_head.body_length = 0;
  msg_head.blob_length = 0;
  msg_head.pkt_type = 0;

  return z_ipc_msg_writer_open(NULL, &msg_head, writer, memory);
}

static void __msg_push (z_memory_t *memory,
                        metrics_rpc_task_t *task,
                        z_ipc_msg_t *msg,
                        z_dbuf_writer_t *writer)
{
  Z_LOG_INFO("COMPLETE SEND MESSAGE %u", writer->size);
  z_ipc_msg_writer_close(msg, writer, writer->size, 0);

  z_ipc_msg_client_push(task->client, msg);
  z_ipc_client_set_writable(task->client, 1);

  z_memory_free(memory, metrics_rpc_task_t, task,
                sizeof(metrics_rpc_task_t) - 1 + task->request_len);
}

static uint16_t __write_histogram (z_dbuf_writer_t *writer,
                                   const z_histogram_t *histo)
{
  uint64_t events[32];
  uint64_t bounds[32];
  uint16_t *ptr[2];
  uint32_t size;
  int i, h;

  for (i = 0, h = 0; histo->max > histo->bounds[i]; ++i) {
    if (histo->events[i] > 0) {
      bounds[h  ] = histo->bounds[i];
      events[h++] = histo->events[i];
    }
  }

  if (histo->events[i] > 0) {
    bounds[h  ] = histo->max;
    events[h++] = histo->events[i];
  }

  size = writer->size;
  z_rpc_write_field_count(writer, 2);
  ptr[0] = metrics_histogram_mark16_bounds(writer);
  ptr[1] = metrics_histogram_mark16_events(writer);
  *(ptr[0]) = z_rpc_write_u64_list(writer, bounds, h);
  *(ptr[1]) = z_rpc_write_u64_list(writer, events, h);

  return((writer->size - size) & 0xffff);
}

static uint16_t __write_iopoll_load (z_dbuf_writer_t *writer,
                                     const z_iopoll_load_t *ioload)
{
  uint8_t *ptr[3];
  uint32_t size;

  size = writer->size;
  z_rpc_write_field_count(writer, 4);
  metrics_iopoll_load_write_max_events(writer, ioload->max_events);
  ptr[0] = metrics_iopoll_load_mark8_events(writer);
  ptr[1] = metrics_iopoll_load_mark8_idle(writer);
  ptr[2] = metrics_iopoll_load_mark8_active(writer);

  *(ptr[0]) = z_rpc_write_u8_list(writer,  ioload->events, 6) & 0xff;
  *(ptr[1]) = z_rpc_write_u32_list(writer, ioload->idle,   6) & 0xff;
  *(ptr[2]) = z_rpc_write_u32_list(writer, ioload->active, 6) & 0xff;

  return((writer->size - size) & 0xffff);
}

static void __metrics_info (z_task_t *vtask) {
  metrics_rpc_task_t *task = z_container_of(vtask, metrics_rpc_task_t, __task__);
  z_dbuf_writer_t writer;
  z_memory_t *memory;
  z_ipc_msg_t *msg;

  memory = z_global_memory();
  msg = __msg_from_task(memory, task, &writer);
  if (Z_MALLOC_IS_NULL(msg)) {
    /* TODO */
    z_iopoll_entity_close(Z_IOPOLL_ENTITY(task->client), 1);
    return;
  }

  z_rpc_write_field_count(&writer, 2);
  metrics_info_resp_write_uptime(&writer, z_global_uptime());
  metrics_info_resp_write_core_count(&writer, z_global_conf()->ncores);

  __msg_push(memory, task, msg, &writer);
}

static void __metrics_rusage (z_task_t *vtask) {
  metrics_rpc_task_t *task = z_container_of(vtask, metrics_rpc_task_t, __task__);
  z_dbuf_writer_t writer;
  struct rusage usage;
  z_memory_t *memory;
  z_ipc_msg_t *msg;

  memory = z_global_memory();
  msg = __msg_from_task(memory, task, &writer);
  if (Z_MALLOC_IS_NULL(msg)) {
    /* TODO */
    z_iopoll_entity_close(Z_IOPOLL_ENTITY(task->client), 1);
    return;
  }

  /* Get System Resources Usage */
  if (getrusage(RUSAGE_SELF, &usage)) {
    perror("getrusage()");
    /* TODO */
    z_iopoll_entity_close(Z_IOPOLL_ENTITY(task->client), 1);
    return;
  }


  z_rpc_write_field_count(&writer, 9);

  /* Stack CPU: ru_utime, ru_stime */
  metrics_rusage_resp_write_utime(&writer, z_timeval_to_micros(&(usage.ru_utime)));
  metrics_rusage_resp_write_stime(&writer, z_timeval_to_micros(&(usage.ru_stime)));

  /* Memory */
  metrics_rusage_resp_write_maxrss(&writer, usage.ru_maxrss << 10);

  /* Stack I/O: ru_inblock, ru_oublock */
  metrics_rusage_resp_write_minflt(&writer, usage.ru_minflt);
  metrics_rusage_resp_write_majflt(&writer, usage.ru_majflt);
  metrics_rusage_resp_write_inblock(&writer, usage.ru_inblock);
  metrics_rusage_resp_write_oublock(&writer, usage.ru_oublock);

  /* Stack Switch: ru_nvcsw, ru_nivcsw */
  metrics_rusage_resp_write_nvcsw(&writer, usage.ru_nvcsw);
  metrics_rusage_resp_write_nivcsw(&writer, usage.ru_nivcsw);

  __msg_push(memory, task, msg, &writer);
}

static void __metrics_memory (z_task_t *vtask) {
  metrics_rpc_task_t *task = z_container_of(vtask, metrics_rpc_task_t, __task__);
  const z_allocator_t *allocator;
  const z_memory_t *core_memory;
  z_dbuf_writer_t writer;
  uint16_t *histo_vlen;
  z_memory_t *memory;
  z_ipc_msg_t *msg;
  uint32_t core_id;

  if (metrics_memory_req_read_core_id(task->request, &core_id) < 0) {
    /* TODO: send error instead of killing the client */
    z_iopoll_entity_close(Z_IOPOLL_ENTITY(task->client), 1);
    return;
  }

  if (Z_UNLIKELY(core_id >= z_global_conf()->ncores)) {
    /* TODO: send error instead of killing the client */
    z_iopoll_entity_close(Z_IOPOLL_ENTITY(task->client), 1);
    return;
  }

  memory = z_global_memory();
  msg = __msg_from_task(memory, task, &writer);
  if (Z_MALLOC_IS_NULL(msg)) {
    /* TODO */
    z_iopoll_entity_close(Z_IOPOLL_ENTITY(task->client), 1);
    return;
  }

  core_memory = z_global_memory_at(core_id);
  allocator = core_memory->allocator;

  z_rpc_write_field_count(&writer, 5);
  metrics_memory_resp_write_pool_used(&writer, allocator->pool_used);
  metrics_memory_resp_write_pool_alloc(&writer, allocator->pool_alloc);
  metrics_memory_resp_write_extern_used(&writer, allocator->extern_used);
  metrics_memory_resp_write_extern_alloc(&writer, allocator->extern_alloc);

  histo_vlen  = metrics_memory_resp_mark16_histogram(&writer);
  *histo_vlen = __write_histogram(&writer, &(core_memory->histo));

  __msg_push(memory, task, msg, &writer);
}

static void __metrics_iopoll (z_task_t *vtask) {
  metrics_rpc_task_t *task = z_container_of(vtask, metrics_rpc_task_t, __task__);
  const z_iopoll_stats_t *iostat;
  z_dbuf_writer_t writer;
  z_memory_t *memory;
  uint16_t *ptrs[6];
  z_ipc_msg_t *msg;
  uint32_t core_id;

  if (metrics_iopoll_req_read_core_id(task->request, &core_id) < 0) {
    /* TODO: send error instead of killing the client */
    z_iopoll_entity_close(Z_IOPOLL_ENTITY(task->client), 1);
    return;
  }

  if (Z_UNLIKELY(core_id >= z_global_conf()->ncores)) {
    /* TODO: send error instead of killing the client */
    z_iopoll_entity_close(Z_IOPOLL_ENTITY(task->client), 1);
    return;
  }

  memory = z_global_memory();
  msg = __msg_from_task(memory, task, &writer);
  if (Z_MALLOC_IS_NULL(msg)) {
    /* TODO */
    z_iopoll_entity_close(Z_IOPOLL_ENTITY(task->client), 1);
    return;
  }

  iostat = z_global_iopoll_stats_at(core_id);

  z_rpc_write_field_count(&writer, 6);
  ptrs[0] = metrics_iopoll_resp_mark16_io_load(&writer);
  ptrs[1] = metrics_iopoll_resp_mark16_io_wait(&writer);
  ptrs[2] = metrics_iopoll_resp_mark16_io_read(&writer);
  ptrs[3] = metrics_iopoll_resp_mark16_io_write(&writer);
  ptrs[4] = metrics_iopoll_resp_mark16_event(&writer);
  ptrs[5] = metrics_iopoll_resp_mark16_timer(&writer);

  *(ptrs[0]) = __write_iopoll_load(&writer, &(iostat->ioload));
  *(ptrs[1]) = __write_histogram(&writer, &(iostat->iowait));
  *(ptrs[2]) = __write_histogram(&writer, &(iostat->ioread));
  *(ptrs[3]) = __write_histogram(&writer, &(iostat->iowrite));
  *(ptrs[4]) = __write_histogram(&writer, &(iostat->event));
  *(ptrs[5]) = __write_histogram(&writer, &(iostat->timeout));

  __msg_push(memory, task, msg, &writer);
}

static const metrics_rpc_server_tasks_t __metrics_tasks = {
  .metrics_info     = __metrics_info,
  .metrics_rusage   = __metrics_rusage,
  .metrics_memory   = __metrics_memory,
  .metrics_iopoll   = __metrics_iopoll,
};

/* ============================================================================
 *  IPC Protocol
 */
static int __client_connected (z_ipc_client_t *ipc_client) {
  struct metrics_client *metrics = Z_CAST(struct metrics_client, ipc_client);
  Z_LOG_INFO("metrics client connected %p", metrics);
  return(0);
}

static int __client_disconnected (z_ipc_client_t *ipc_client) {
  struct metrics_client *metrics = Z_CAST(struct metrics_client, ipc_client);
  Z_LOG_INFO("metrics client disconnected %p", metrics);
  return(0);
}

static int __client_msg_alloc (z_ipc_msg_client_t *ipc_client,
                               z_ipc_msg_head_t *msg_head)
{
  struct metrics_client *cmetrics = Z_CAST(struct metrics_client, ipc_client);
  metrics_rpc_task_t *task;
  z_task_func_t func;

  Z_LOG_DEBUG("msg_id: %"PRIu64" msg_type: %"PRIu32
              " body_len: %"PRIu16" blob_length: %"PRIu32,
              msg_head->msg_id, msg_head->msg_type,
              msg_head->body_length, msg_head->blob_length);

  /* kill the client if the message is too big */
  if (Z_UNLIKELY(msg_head->body_length > 0xff || msg_head->blob_length > 0)) {
    Z_LOG_TRACE("message is too big: msg_id=%"PRIu64" body_length=%u blob_length=%u",
                msg_head->msg_id, msg_head->body_length, msg_head->blob_length);
    return(-1);
  }

  /* ...what are you sending? */
  func = metrics_rpc_server_task_func_by_type(&__metrics_tasks, msg_head->msg_type);
  if (Z_UNLIKELY(func == NULL)) {
    Z_LOG_TRACE("invalid message type: msg_id=%"PRIu64" msg_type=%u body_length=%u blob_length=%u",
                msg_head->msg_id, msg_head->msg_type, msg_head->body_length, msg_head->blob_length);
    return(-1);
  }

  /* alloc new task */
  task = z_memory_alloc(z_global_memory(), metrics_rpc_task_t,
                        sizeof(metrics_rpc_task_t) - 1 + msg_head->body_length);
  if (Z_MALLOC_IS_NULL(task))
    return(-1);

  z_task_reset(&(task->__task__), func);
  task->client = ipc_client;
  task->msg_id = msg_head->msg_id;
  task->msg_type = msg_head->msg_type;
  task->request_len = msg_head->body_length;
  cmetrics->task = task;

  ipc_client->rdbuf.body_buffer = task->request;
  ipc_client->rdbuf.blob_buffer = NULL;
  return(0);
}

static int __client_msg_exec (z_ipc_msg_client_t *ipc_client,
                              z_ipc_msg_head_t *msg_head)
{
  struct metrics_client *cmetrics = Z_CAST(struct metrics_client, ipc_client);
  Z_LOG_DEBUG("msg_id: %"PRIu64" msg_type: %"PRIu32
              " body_len: %"PRIu16" blob_length: %"PRIu32,
              msg_head->msg_id, msg_head->msg_type,
              msg_head->body_length, msg_head->blob_length);
  z_channel_add_task(&(cmetrics->task->__task__.vtask));
  cmetrics->task = NULL;
  return(0);
}

const z_ipc_protocol_t metrics_tcp_protocol = {
  /* raw-client protocol */
  .read         = NULL,
  .write        = NULL,

  /* client protocol */
  .connected    = __client_connected,
  .disconnected = __client_disconnected,

  /* server protocol */
  .uevent       = NULL,
  .timeout      = NULL,
  .accept       = z_ipc_accept_tcp,
  .bind         = z_ipc_bind_tcp,
  .unbind       = NULL,
  .setup        = NULL,
};

const z_ipc_msg_protocol_t metrics_msg_protocol = {
  .msg_alloc   = __client_msg_alloc,
  .msg_parse   = NULL,
  .msg_exec    = __client_msg_exec,
  .msg_respond = NULL,
  .msg_free    = NULL,
};
