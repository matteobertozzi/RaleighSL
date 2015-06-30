#include <zcl/time.h>
#include <zcl/msg.h>
#include <zcl/fd.h>

#include <unistd.h>

struct client {
  uint8_t ringbuf[16];
  int pipefd[2];
};

int __test_alloc (void *udata, z_msg_ibuf_t *ibuf, uint32_t length) {
  struct client *client = Z_CAST(struct client, udata);
  //printf("ALLOC %u\n", length);
  memset(client->ringbuf, 0, 16);
  ibuf->buf.blob = client->ringbuf;
  return(0);
}

int __test_append (void *udata, z_msg_ibuf_t *ibuf,
                   const uint8_t *buffer, uint32_t size) {
  //printf("APPEND %u\n", size);
  memcpy(ibuf->buf.blob, buffer, size);
  return(0);
}

int __test_publish (void *udata, z_msg_ibuf_t *ibuf) {
#if 0
  printf("PUBLISH len=%u type=%d blob=%s\n",
         ibuf->available, ibuf->pkg_type, ibuf->buf.blob + 2);
#endif
  return(0);
}

const z_msg_protocol_t __test_proto = {
  .alloc    = __test_alloc,
  .append   = __test_append,
  .publish  = __test_publish,
};

/* ============================================================================
 *  I/O Raw
 */
static ssize_t __iopoll_entity_raw_read (void *udata, void *buffer, size_t bufsize) {
  return z_fd_read(Z_CAST(struct client, udata)->pipefd[0], buffer, bufsize);
}

static ssize_t __iopoll_entity_raw_write (void *udata, const void *buffer, size_t bufsize) {
  return z_fd_write(Z_CAST(struct client, udata)->pipefd[1], buffer, bufsize);
}

const z_io_seq_vtable_t z_iopoll_entity_raw_io_seq_vtable = {
  .read  = __iopoll_entity_raw_read,
  .write = __iopoll_entity_raw_write,
};


static void __test_partial_steps (struct client *client) {
  z_msg_ibuf_t ibuf;
  char buffer[8];

  z_msg_ibuf_init(&ibuf);

  /* frame first byte */
  buffer[0] = (1 << 4) | 0 << 2 | 0;
  z_fd_write(client->pipefd[1], buffer, 1);
  z_msg_ibuf_read(&ibuf, &z_iopoll_entity_raw_io_seq_vtable, &__test_proto, client);

  /* frame second byte */
  buffer[0] = 5;
  z_fd_write(client->pipefd[1], buffer, 1);
  z_msg_ibuf_read(&ibuf, &z_iopoll_entity_raw_io_seq_vtable, &__test_proto, client);

  /* frame content, first 3 bytes */
  z_fd_write(client->pipefd[1], "hel", 3);
  z_msg_ibuf_read(&ibuf, &z_iopoll_entity_raw_io_seq_vtable, &__test_proto, client);

  /* frame content, last 2 bytes */
  z_fd_write(client->pipefd[1], "lo", 2);
  z_msg_ibuf_read(&ibuf, &z_iopoll_entity_raw_io_seq_vtable, &__test_proto, client);
}

static void __test_batch_body (struct client *client) {
  z_msg_ibuf_t ibuf;
  char buffer[16];
  int i;

  z_msg_ibuf_init(&ibuf);

  for (i = 0; i < 16; i += 4) {
    buffer[i + 0] = (((i >> 1) + 1) << 4) | 0 << 2 | 0;
    buffer[i + 1] = 2;
    buffer[i + 2] = 'A' + i;
    buffer[i + 3] = 'B' + i;
  }

  z_fd_write(client->pipefd[1], buffer, 16);
  z_msg_ibuf_read(&ibuf, &z_iopoll_entity_raw_io_seq_vtable, &__test_proto, client);
}

static void __test_batch_nobody (struct client *client) {
  z_msg_ibuf_t ibuf;
  char buffer[8];
  int i;

  z_msg_ibuf_init(&ibuf);

  for (i = 0; i < 8; i += 2) {
    buffer[i + 0] = (((i >> 1) + 1) << 4) | 0 << 2 | 0;
    buffer[i + 1] = 0;
  }

  z_fd_write(client->pipefd[1], buffer, 8);
  z_msg_ibuf_read(&ibuf, &z_iopoll_entity_raw_io_seq_vtable, &__test_proto, client);
}

int main (int argc, char **argv) {
  struct client client;

  if (pipe(client.pipefd) < 0) {
    perror("pipe()");
    return(1);
  }

  if (z_fd_set_blocking(client.pipefd[0], 0)) {
    perror("z_fd_set_blocking()");
    close(client.pipefd[0]);
    close(client.pipefd[1]);
    return(1);
  }

  #define NRUNS (1000000)
  uint64_t st = z_time_millis();
  int i;
  for (i = 0; i < NRUNS; ++i) {
    __test_partial_steps(&client);
    __test_batch_nobody(&client);
    __test_batch_body(&client);
  }
  float elapsed = (z_time_millis() - st) / 1000.0f;
  printf("%.2fG msg/sec\n", (((1 + 4 + 4) * NRUNS) / elapsed) / 1000000.0f);

  close(client.pipefd[0]);
  close(client.pipefd[1]);
  return(0);
}
