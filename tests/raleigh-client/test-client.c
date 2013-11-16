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

#include <zcl/threading.h>
#include <zcl/time.h>

#include <stdio.h>

#include <raleigh-client/raleigh.h>

#define NREQUESTS    ((uint64_t)200000)
#define NTHREADS     1

static int __test_client (void) {
  raleigh_client_t *client;

  if ((client = raleigh_tcp_connect("localhost", "11215", 0)) == NULL)
    return(1);

  do {
    uint64_t req_id = 0;
    uint64_t st, et;
    st = z_time_nanos();
    while (req_id < NREQUESTS) {
      raleigh_ping(client);
      req_id++;
    }
    et = z_time_nanos();
    printf("server_ping reqs %"PRIu64" %.3fsec (%.2freq/sec)\n", req_id, (et - st) / 1000000000.0f,
           req_id / ((et - st) / 1000000000.0f));
  } while (0);

  return(0);
}

static void *__execute_test (void *argc) {
  __test_client();
  return(NULL);
}

static int __test_clients (void) {
  uint64_t et, st = z_time_nanos();
  z_thread_t thread[NTHREADS];
  int i;

  for (i = 0; i < NTHREADS; ++i) {
    z_thread_start(&(thread[i]), __execute_test, NULL);
  }

  for (i = 0; i < NTHREADS; ++i) {
    z_thread_join(&(thread[i]));
  }

  et = z_time_nanos();
  printf("%d clients estimate avg-reqs %"PRIu64" %.3fsec (%.2freq/sec)\n", NTHREADS,
         ((NREQUESTS) * NTHREADS), (et - st) / 1000000000.0f,
         ((NREQUESTS) * NTHREADS) / ((et - st) / 1000000000.0f));
  return(0);
}

int main (int argc, char **argv) {
  /* Initialize global context */
  raleigh_initialize();

  __test_clients();

  /* Uninitialize global context */
  raleigh_uninitialize();
  return(0);
}