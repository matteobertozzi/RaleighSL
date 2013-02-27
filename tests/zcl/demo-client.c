#include <signal.h>
#include <stdio.h>

#include <zcl/memory.h>
#include <zcl/iopoll.h>
#include <zcl/socket.h>
#include <zcl/time.h>
#include <zcl/fd.h>

struct z_test_client {
    __Z_IOPOLL_ENTITY__
    z_iopoll_t *iopoll;
    unsigned int read;
    unsigned int write;
    uint64_t time;
    uint64_t dt;
};

#define HEX         "0123456789abcdef"
#define BLOB32      HEX HEX
#define BLOB64      BLOB32 BLOB32
#define BLOB128     BLOB64 BLOB64
#define BLOB256     BLOB128 BLOB128
#define BLOB512     BLOB256 BLOB256
#define BLOB1024    BLOB512 BLOB512

int __test_client_read (z_iopoll_entity_t *entity) {
  struct z_test_client *client = (struct z_test_client *)entity;
  char buffer[8192];
  uint64_t dt;
  ssize_t rd;

  while ((rd = read(entity->fd, buffer, sizeof(buffer))) > 0)
    client->read += rd;

  if ((dt = z_time_millis() - client->dt) > 2000) {
      float sec;
      client->dt = z_time_millis();
      sec = (client->dt - client->time) / 1000.0f;
      printf("%p: Read %ld %.2fMiB/sec Write %.2fMiB/sec\n", entity, rd,
              (client->read / sec) / 1024.0f / 1024.0f,
              (client->write / sec) / 1024.0f / 1024.0f);
  }

  z_iopoll_set_writable(client->iopoll, entity, 1);
  return(0);
}

int __test_client_write (z_iopoll_entity_t *entity) {
  struct z_test_client *client = (struct z_test_client *)entity;
  ssize_t wr;

  if ((wr = write(entity->fd, BLOB1024, 1024)) > 0)
    client->write += wr;

  z_iopoll_set_writable(client->iopoll, entity, 0);
  return(0);
}

void __test_client_close (z_iopoll_entity_t *entity) {
}

static const z_vtable_iopoll_entity_t __client_vtable = {
  .read  = __test_client_read,
  .write = __test_client_write,
  .close = __test_client_close,
};

static int __is_running = 1;
static void __signal_handler (int signum) {
    __is_running = 0;
}

#define NCLIENT    1
int main (int argc, char **argv) {
  struct z_test_client client[NCLIENT];
  z_memory_t memory;
  z_iopoll_t iopoll;
  int i;

  /* Initialize signals */
  signal(SIGINT, __signal_handler);

  /* Initialize memory */
  z_system_memory_init(&memory);

  /* Initialize I/O Poll */
  if (z_iopoll_open(&iopoll, &memory, NULL)) {
      fprintf(stderr, "z_iopoll_open(): failed\n");
      return(1);
  }

  for (i = 0; i < NCLIENT; ++i) {
      int fd = z_socket_tcp_connect("localhost", "11214", NULL);
      z_socket_tcp_set_nodelay(fd);
      z_fd_set_blocking(fd, 0);
      z_iopoll_entity_init(Z_IOPOLL_ENTITY(&(client[i])), &__client_vtable, fd);
      z_iopoll_set_writable(&iopoll, Z_IOPOLL_ENTITY(&(client[i])), 1);
      client[i].iopoll = &iopoll;
      client[i].read = 0;
      client[i].write = 0;
      client[i].dt = client[i].time = z_time_millis();
      z_iopoll_add(&iopoll, Z_IOPOLL_ENTITY(&(client[i])));
  }

  /* Start spinning... */
  z_iopoll_poll(&iopoll, &__is_running, -1);

  /* ...and we're done */
  z_iopoll_close(&iopoll);
  return(0);
}
