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

#include <zcl/system.h>
#include <zcl/coding.h>
#include <zcl/string.h>
#include <zcl/iovec.h>
#include <zcl/debug.h>
#include <zcl/time.h>
#include <zcl/ipc.h>

#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>

#if defined(Z_SYS_HAS_MALLINFO)
  #include <malloc.h>
#endif

#include "rpc/generated/stats_server.h"
#include "server.h"

/* ============================================================================
 *  Stats RPC Protocol
 */
static int __memusage (z_rpc_ctx_t *ctx,
                       struct memusage_request *req,
                       struct memusage_response *resp)
{
  struct stats_client *client = STATS_CLIENT(ctx->client);
#if defined(Z_SYS_HAS_MALLINFO)
  struct mallinfo mminfo = mallinfo();

  resp->arena    = mminfo.arena;        /* Non-mmapped space allocated (bytes) */
  memusage_response_set_arena(resp);
  resp->ordblks  = mminfo.ordblks;            /* Number of free chunks */
  memusage_response_set_ordblks(resp);
  resp->smblks   = mminfo.smblks;             /* Number of free fastbin blocks */
  memusage_response_set_smblks(resp);
  resp->hblks    = mminfo.hblks;              /* Number of mmapped regions */
  memusage_response_set_hblks(resp);
  resp->hblkhd   = mminfo.hblkhd;       /* Space allocated in mmapped regions (bytes) */
  memusage_response_set_hblkhd(resp);
  resp->usmblks  = mminfo.usmblks;      /* Maximum total allocated space (bytes) */
  memusage_response_set_usmblks(resp);
  resp->fsmblks  = mminfo.fsmblks;      /* Space in freed fastbin blocks (bytes) */
  memusage_response_set_fsmblks(resp);
  resp->uordblks = mminfo.uordblks;     /* Total allocated space (bytes) */
  memusage_response_set_uordblks(resp);
  resp->fordblks = mminfo.fordblks;     /* Total free space (bytes) */
  memusage_response_set_fordblks(resp);
  resp->keepcost = mminfo.keepcost;     /* Top-most, releasable space (bytes) */
  memusage_response_set_keepcost(resp);

  resp->sysused = z_system_memory_used();
  memusage_response_set_sysused(resp);
  resp->sysfree = z_system_memory_free();
  memusage_response_set_sysfree(resp);
#endif
  return(stats_rpc_server_push_response(ctx, &(client->msgbuf)));
}

static int __rusage (z_rpc_ctx_t *ctx,
                     struct rusage_request *req,
                     struct rusage_response *resp)
{
  struct stats_client *client = STATS_CLIENT(ctx->client);
  struct rusage usage;

  /* Get System Resources Usage */
  if (getrusage(RUSAGE_SELF, &usage) < 0) {
    /* TODO: Stats Rpc Server Push Failure */
    return(1);
  }

  /* Stack CPU: ru_utime, ru_stime */
  resp->utime = z_timeval_to_micros(&(usage.ru_utime));
  rusage_response_set_utime(resp);
  resp->stime = z_timeval_to_micros(&(usage.ru_stime));
  rusage_response_set_stime(resp);

  /* Memory */
  resp->maxrss = (usage.ru_maxrss << 10);
  rusage_response_set_maxrss(resp);

  /* Stack I/O: ru_inblock, ru_oublock */
  resp->minflt  = usage.ru_minflt;
  rusage_response_set_minflt(resp);
  resp->majflt  = usage.ru_majflt;
  rusage_response_set_majflt(resp);
  resp->inblock = usage.ru_inblock;
  rusage_response_set_inblock(resp);
  resp->oublock = usage.ru_oublock;
  rusage_response_set_oublock(resp);

  /* Stack Switch: ru_nvcsw, ru_nivcsw */
  resp->nvcsw = usage.ru_nvcsw;
  rusage_response_set_nvcsw(resp);
  resp->nivcsw = usage.ru_nivcsw;
  rusage_response_set_nivcsw(resp);

  return(stats_rpc_server_push_response(ctx, &(client->msgbuf)));
}

static int __iopoll (z_rpc_ctx_t *ctx,
                     struct iopoll_request *req,
                     struct iopoll_response *resp)
{
  struct stats_client *client = STATS_CLIENT(ctx->client);
  int i;
  for (i = 0; i < Z_IOPOLL_ENGINES; ++i) {
    const z_iopoll_stats_t *stats = &(z_ipc_client_iopoll(ctx->client)->engines[i].stats);
    z_array_push_back_copy(&(resp->waits), &(stats->iowait.sum));
    z_array_push_back_copy(&(resp->reads), &(stats->ioread.sum));
    z_array_push_back_copy(&(resp->writes), &(stats->iowrite.sum));
  }
  iopoll_response_set_waits(resp);
  iopoll_response_set_reads(resp);
  iopoll_response_set_writes(resp);
  return(stats_rpc_server_push_response(ctx, &(client->msgbuf)));
}

/* ============================================================================
 *  Stats RPC Protocol
 */
static const struct stats_rpc_server __stats_protocol = {
  .rusage   = __rusage,
  .memusage = __memusage,
  .iopoll   = __iopoll,
};

static int __client_msg_parse (z_iopoll_entity_t *ipc_client, const struct iovec iov[2]) {
  return(stats_rpc_server_parse(&__stats_protocol, Z_IPC_CLIENT(ipc_client), iov));
}

/* ============================================================================
 *  Stats IPC Protocol
 */
static int __client_connected (z_ipc_client_t *ipc_client) {
  struct stats_client *client = (struct stats_client *)ipc_client;
  z_ipc_msgbuf_open(&(client->msgbuf), 1024);
  Z_LOG_DEBUG("Stats client connected");
  client->id = 0;
  return(0);
}

static void __client_disconnected (z_ipc_client_t *ipc_client) {
  struct stats_client *client = (struct stats_client *)ipc_client;
  Z_LOG_DEBUG("Stats client disconnected");
  z_ipc_msgbuf_close(&(client->msgbuf));
}

static int __client_read (z_ipc_client_t *ipc_client) {
  struct stats_client *client = (struct stats_client *)ipc_client;
  Z_LOG_DEBUG("Stats client read");
  return(z_ipc_msgbuf_fetch(&(client->msgbuf), Z_IOPOLL_ENTITY(ipc_client), __client_msg_parse));
}

static int __client_write (z_ipc_client_t *ipc_client) {
  struct stats_client *client = (struct stats_client *)ipc_client;
  return(z_ipc_msgbuf_flush(&(client->msgbuf), z_ipc_client_iopoll(client), Z_IOPOLL_ENTITY(ipc_client)));
}

const z_ipc_protocol_t stats_protocol = {
  /* server protocol */
  .bind         = z_ipc_bind_tcp,
  .accept       = z_ipc_accept_tcp,
  .setup        = NULL,

  /* client protocol */
  .connected    = __client_connected,
  .disconnected = __client_disconnected,
  .read         = __client_read,
  .write        = __client_write,
};
