#include <zcl/dbuffer.h>
#include <zcl/global.h>
#include <zcl/debug.h>

static void test_validate (void) {
  uint8_t buffer[1024];
  z_dbuf_writer_t writer;
  z_dbuf_reader_t reader;
  struct iovec iovs[1];
  int boff, blen;
  uint64_t i;
  int niovs;

  boff = 0;
  blen = 0;

  z_dbuf_writer_open(&writer, z_global_memory(), NULL, 0, 0);
  for (i = 0; i < 64; ++i) {
    z_dbuf_writer_add(&writer, &i, 8);
    memcpy(buffer + boff, &i, 8);
    boff += 8;
  }

  boff = 0;
  z_dbuf_reader_open(&reader, z_global_memory(), writer.head, 0, 0);
  for (i = 0; i < 64; ++i) {
    niovs = z_dbuf_reader_get_iovs(&reader, iovs, 1);
    Z_ASSERT(niovs == 1, "%"PRIu64" expected one iov %d", i, niovs);

    Z_ASSERT(memcmp(buffer + boff, iovs[0].iov_base, iovs[0].iov_len) == 0,
             "failed data comparison");

    boff += 8;
    z_dbuf_reader_remove(&reader, 8);
  }
}

int main (int argc, char **argv) {
  /* Initialize global context */
  if (z_global_context_open_default(1)) {
    Z_LOG_FATAL("z_iopoll_open(): failed\n");
    return(1);
  }

  test_validate();

  /* ...and we're done */
  z_global_context_close();
  return(0);
}