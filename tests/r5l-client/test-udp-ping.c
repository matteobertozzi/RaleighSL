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

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <zcl/int-coding.h>
#include <zcl/socket.h>
#include <zcl/humans.h>
#include <zcl/debug.h>
#include <zcl/time.h>

#define PIPELINE        1
//#define NLOOPS          z_align_up(100000, PIPELINE)
#define NLOOPS          600000
#define NCLIENTS        4

#define BUFSIZE         PIPELINE * 32
//#define HOST            "192.168.0.6"
#define HOST            "127.0.0.1"
#define PORT            "11123"

void __client_ping (int client_id, const void *address, const void *port) {
  struct sockaddr_storage addr;
  uint8_t buffer[BUFSIZE];
  char sbuffer[64];
  uint64_t req_id;
  uint64_t stime;
  uint8_t *pbuf;
  float etime;
  int sock;

  if ((sock = z_socket_udp_connect(address, port, &addr)) < 0) {
    perror("z_socket_udp_connect()");
    return;
  }

  const uint64_t msg_len = 0;
  const uint64_t msg_type = 90;

  stime = z_time_micros();
  for (req_id = 0; req_id < NLOOPS; ) {
    int rd;
    int i;

    pbuf = buffer;
    for (i = 0; i < PIPELINE; ++i) {
if (0) {
      uint8_t h_msg_len = z_uint64_size(msg_len);
      uint8_t h_msg_type = z_uint64_size(msg_type);
      uint8_t h_req_id = z_uint64_size(req_id);

      *pbuf++ = (0 << 3) | h_msg_len;
      *pbuf++ = (h_msg_type << 5) | (h_req_id << 2) | 0;
      z_uint_encode(pbuf, h_msg_len, msg_len);   pbuf += h_msg_len;
      z_uint_encode(pbuf, h_msg_type, msg_type); pbuf += h_msg_type;
      z_uint_encode(pbuf, h_req_id, req_id);     pbuf += h_req_id;
}
      memcpy(pbuf, "PING\n", 5);
      pbuf += 5;

      ++req_id;
    }

    if (z_socket_udp_send(sock, &addr, buffer, pbuf - buffer, 0) <= 0) {
      perror("z_socket_udp_send()");
      return;
    }

    pbuf = buffer;
    rd = z_socket_udp_recv(sock, &addr, pbuf, sizeof(buffer) - (pbuf - buffer), 0, 500);
    if (rd < 0) {
      perror("read()");
      return;
    }
  }
  etime = z_usec_to_sec(z_time_micros() - stime);

  close(sock);

  z_human_ops(sbuffer, sizeof(sbuffer), req_id / etime);
  fprintf(stderr, "%03d %"PRIu64" Test %.2fsec %.2freq/sec %sreq/sec\n",
                  client_id, req_id, etime, req_id / etime, sbuffer);
}

#define __test_func       __client_ping

int main (int argc, char **argv) {
  pid_t pids[NCLIENTS];
  char buffer[64];
  uint64_t stime;
  float etime;
  int i;

  stime = z_time_micros();
  for (i = 0; i < NCLIENTS - 1; ++i) {
    pid_t pid;
    if ((pid = fork()) == 0) {
      __test_func(i, HOST, PORT);
      return(0);
    } else {
      pids[i] = pid;
    }
  }

  __test_func(i, HOST, PORT);
  for (i = 0; i < NCLIENTS - 1; ++i) {
    waitpid(pids[i], NULL, 0);
  }
  etime = z_usec_to_sec(z_time_micros() - stime);
  z_human_ops(buffer, sizeof(buffer), (NLOOPS * NCLIENTS) / etime);
  fprintf(stderr, "Total %d Test %.2fsec %.2freq/sec (%s/sec)\n",
                  NCLIENTS, etime, (NLOOPS * NCLIENTS) / etime, buffer);

  return(0);
}
