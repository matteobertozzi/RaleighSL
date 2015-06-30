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
#include <zcl/time.h>

#define PIPELINE        1
//#define NLOOPS          z_align_up(100000, PIPELINE)
#define NLOOPS          600000
#define NCLIENTS        4

#define BUFSIZE         PIPELINE * 80
//#define HOST            "192.168.0.6"
#define HOST            "localhost"
#define PORT            "11124"

void __client_ping (int client_id, const void *address, const void *port) {
  uint8_t buffer[BUFSIZE];
  char sbuffer[64];
  uint64_t req_id;
  uint64_t stime;
  uint8_t *pbuf;
  float etime;
  int sock;

  if ((sock = z_socket_tcp_connect(address, port, NULL)) < 0)
    return;

  stime = z_time_micros();
  for (req_id = 0; req_id < NLOOPS; ) {
    ssize_t rd;
    int i;

    pbuf = buffer;
    for (i = 0; i < PIPELINE; ++i) {
#if 1
      *pbuf++ = (1 << 4) | 0 << 2 | 0;
      *pbuf++ = 64;
      z_uint_encode(pbuf, 8, req_id);
      memset(pbuf + 8, 0xAA, 56);
      pbuf += 64;
#else
      memcpy(pbuf, "PING\n", 5);
      pbuf += 5;
#endif
      ++req_id;
    }

    if (write(sock, buffer, pbuf - buffer) != (pbuf - buffer)) {
      perror("write()");
      return;
    }

    pbuf = buffer;
    rd = read(sock, pbuf, sizeof(buffer));
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

void __client_throughput (int client_id, const void *address, const void *port) {
  uint8_t buffer[2 << 20];
  char sbuffer[64];
  uint64_t req_id;
  uint64_t tsize;
  uint64_t stime;
  uint8_t *pbuf;
  float etime;
  int sock;

  if ((sock = z_socket_tcp_connect(address, port, NULL)) < 0)
    return;

  uint64_t msg_len = 0;
  uint64_t msg_type = 41;

  tsize = 0;
  stime = z_time_micros();
  for (req_id = 0; req_id < NLOOPS; ++req_id) {
    ssize_t rd;

    uint8_t h_msg_len = z_uint64_size(msg_len);
    uint8_t h_msg_type = z_uint64_size(msg_type);
    uint8_t h_req_id = z_uint64_size(req_id);

    pbuf = buffer;
    *pbuf++ = (0 << 3) | h_msg_len;
    *pbuf++ = (h_msg_type << 5) | (h_req_id << 2) | 0;
    z_uint_encode(pbuf, h_msg_len, msg_len);   pbuf += h_msg_len;
    z_uint_encode(pbuf, h_msg_type, msg_type); pbuf += h_msg_type;
    z_uint_encode(pbuf, h_req_id, req_id);     pbuf += h_req_id;
    if (write(sock, buffer, pbuf - buffer) <= 0) {
      perror("write()");
      return;
    }

    rd = read(sock, buffer, sizeof(buffer));
    if (Z_UNLIKELY(rd < 0)) {
      perror("read()");
      return;
    }

    h_msg_len  = buffer[0] & 7;
    h_msg_type = (buffer[1] >> 5) & 7;
    h_req_id   = (buffer[1] >> 2) & 7;
    z_uint64_decode(buffer + 2, h_msg_len, &msg_len);
    tsize += msg_len;
    //fprintf(stderr, "GOT %zd/%zu\n", rd, msg_len);
    msg_len -= rd - (2 + h_msg_len + h_msg_type + h_req_id);
    while (msg_len > 0) {
      rd = read(sock, buffer, sizeof(buffer));
      //fprintf(stderr, "GOT %zd/%zu\n", rd, msg_len);
      if (Z_UNLIKELY(rd < 0)) {
        perror("read()");
        break;
      }
      msg_len -= rd;
    }
  }
  etime = z_usec_to_sec(z_time_micros() - stime);

  close(sock);

  z_human_ops(sbuffer, sizeof(sbuffer), tsize / etime);
  fprintf(stderr, "%03d %"PRIu64" Test %.2fsec %.2fqps %s/sec\n",
                  client_id, req_id, etime, req_id / etime, buffer);
}

#define __test_func       __client_ping
//#define __test_func       __client_throughput

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
