#include <zcl/dbuffer.h>
#include <zcl/global.h>
#include <zcl/debug.h>

static void test_validate (void) {
  uint8_t buffer[1024];
  z_dbuf_writer_t writer;
  z_dbuf_reader_t reader;
  struct iovec iovs[1];
  uint64_t i;
  int niovs;
  int boff;

  boff = 0;
  z_dbuf_writer_init(&writer, z_global_memory(), NULL, 0, 0);
  for (i = 0; i < 64; ++i) {
    uint8_t xbuf[8];
    uint8_t *pbuf;
    pbuf = z_dbuf_writer_next(&writer, xbuf, 8);
    memcpy(pbuf, &i, 8);
    pbuf = z_dbuf_writer_commit(&writer, pbuf, 8);

    memcpy(buffer + boff, &i, 8);
    boff += 8;
  }

  boff = 0;
  z_dbuf_reader_init(&reader, z_global_memory(), writer.head, 0, 0);
  for (i = 0; i < 64; ++i) {
    niovs = z_dbuf_reader_iovs(&reader, iovs, 1);
    Z_ASSERT(niovs == 1, "%"PRIu64" expected one iov %d", i, niovs);

    Z_ASSERT(memcmp(buffer + boff, iovs[0].iov_base, 8) == 0,
             "failed data comparison %d %p: %"PRIu64" vs %"PRIu64, i, iovs[0].iov_base,
             *((uint64_t *)(iovs[0].iov_base)), *((uint64_t *)(buffer + boff)));

    boff += 8;
    z_dbuf_reader_remove(&reader, 8);
  }
  niovs = z_dbuf_reader_iovs(&reader, iovs, 1);
  Z_ASSERT(niovs == 0, "expected zero iov %d", niovs);
}

static void test_validate2 (void) {
  uint8_t buffer[1024];
  z_dbuf_writer_t writer;
  z_dbuf_reader_t reader;
  struct iovec iovs[1];
  int boff, blen;
  uint64_t i;
  int niovs;

  boff = 0;
  blen = 0;

  z_dbuf_writer_init(&writer, z_global_memory(), NULL, 0, 0);
  for (i = 0; i < 64; ++i) {
    uint8_t xbuf[8];
    uint8_t *pbuf;
    pbuf = z_dbuf_writer_next(&writer, xbuf, 8);
    memcpy(pbuf, &i, 8);
    pbuf = z_dbuf_writer_commit(&writer, pbuf, 8);

    memcpy(buffer + boff, &i, 8);
    boff += 8;
    blen += 8;
  }

  boff = 0;
  z_dbuf_reader_init(&reader, z_global_memory(), writer.head, 0, 0);
  while (1) {
    niovs = z_dbuf_reader_iovs(&reader, iovs, 1);
    Z_ASSERT(niovs == (blen != 0), "niovs %d blen %d", niovs, blen);
    if (blen == 0)
      break;

    Z_ASSERT(memcmp(buffer + boff, iovs[0].iov_base, iovs[0].iov_len) == 0,
             "failed data comparison %d", i);

    boff += iovs[0].iov_len;
    blen -= iovs[0].iov_len;
    z_dbuf_reader_remove(&reader, iovs[0].iov_len);
  }
}

int main (int argc, char **argv) {
  /* Initialize global context */
  if (z_global_context_open_default(1)) {
    Z_LOG_FATAL("z_iopoll_open(): failed\n");
    return(1);
  }

  test_validate();
  test_validate2();

  /* ...and we're done */
  z_global_context_close();
  return(0);
}