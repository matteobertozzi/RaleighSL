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

#include <zcl/int-coding.h>
#include <zcl/msg.h>
#include <zcl/fd.h>

#ifndef Z_MSG_RDBUF
  #define Z_MSG_RDBUF   512
#endif

#ifndef Z_MSG_RDBUF_SKIP
  #define Z_MSG_SKIPBUF 2048
#endif

/*
 *              header: 1byte
 *              frame-length: 1-4bytes (expected max 3bytes)
 *              forward-length: 0-3bytes (expected max 2bytes)
 *              forward-data: 0-16M (expected max 64k)
 *              frame-data: 0-4G (expected max 16M)
 *        --- +--------------------------------------------------+ ---
 *         |  | pkg-type 4bit      | fwd-len-b 2bit | len-b 2bit |  |
 *   head  |  +--------------------------------------------------+  |
 *  min 2b |  |                   Frame Length                   |  | Replaced
 *  max 8b |  +--------------------------------------------------+  |  by the
 *         |  |                  Forward Length                  |  |   Proxy
 *        --- +--------------------------------------------------+  |
 *            /                   Forward Data                   /  |
 *            +--------------------------------------------------+ ---
 *            /                                                  /
 *            /                   Frame payload                  /
 *            /                                                  /
 *            +--------------------------------------------------+
 *
 */
static ssize_t __msg_parse (z_msg_ibuf_t *ibuf,
                            uint8_t *buffer, ssize_t bufsize,
                            const z_msg_protocol_t *proto, void *udata)
{
  uint32_t frame_length;
  uint8_t frame_len_bytes;
  uint8_t fwd_len_bytes;
  uint8_t len_bytes;

  /* check for min frame length */
  fwd_len_bytes = (buffer[0] & 12) >> 2;
  frame_len_bytes = 1 + (buffer[0] & 3);
  len_bytes = 1 + frame_len_bytes + fwd_len_bytes;
  if (Z_UNLIKELY(bufsize < len_bytes)) {
    memcpy(ibuf->buf.frame + ibuf->buflen, buffer, bufsize);
    ibuf->buflen += bufsize;
    return(0);
  }

  /* parse frame-length and fwd-length */
  z_uint32_decode(buffer + 1, frame_len_bytes, &frame_length);
  z_uint32_decode(buffer + 1 + frame_len_bytes, fwd_len_bytes, &(ibuf->fwd_length));
  frame_length += len_bytes + ibuf->fwd_length;

  ibuf->pkg_type = buffer[0] >> 2;
  ibuf->framelen = len_bytes;

  /* allocate and copy to ringbuf */
  if (Z_UNLIKELY(proto->alloc(udata, ibuf, frame_length)))
    return(-1);

  /* if we have the full frame parse it */
  ibuf->buflen = 0;
  if (bufsize >= frame_length) {
    ibuf->remaining = 0;
    ibuf->available = frame_length;
    if (ibuf->store) {
      memcpy(ibuf->buf.blob, buffer, frame_length);
    } else {
      proto->append(udata, ibuf, buffer, frame_length);
    }
    proto->publish(udata, ibuf);
    return(bufsize - frame_length);
  }

  /* not enough buffer to end the frame */
  ibuf->remaining = frame_length - bufsize;
  ibuf->available = bufsize;
  if (ibuf->store) {
    memcpy(ibuf->buf.blob, buffer, bufsize);
  } else {
    proto->append(udata, ibuf, buffer, bufsize);
  }
  return(0);
}

/* ============================================================================
 *  PRIVATE fd I/O methods
 */
static int __msg_read_new (z_msg_ibuf_t *ibuf,
                           const z_io_seq_vtable_t *io_proto, int *read_more,
                           const z_msg_protocol_t *proto, void *udata)
{
  uint8_t buffer[Z_MSG_RDBUF];
  uint8_t *pbuffer = buffer;
  size_t avail = Z_MSG_RDBUF;
  ssize_t rd;

  if (ibuf->buflen > 0) {
    memcpy(buffer, ibuf->buf.frame, ibuf->buflen);
    pbuffer += ibuf->buflen;
    avail -= ibuf->buflen;
  } else {
    /* first read */
  }

  rd = io_proto->read(udata, pbuffer, avail);
  if (Z_UNLIKELY(rd < 0))
    return(-1);

  *read_more = (rd == avail);
  //ibuf->rdcount++;

  /* Try to parse the new message */
  pbuffer = buffer;
  while (rd != 0) {
    ssize_t bufsize = rd + ibuf->buflen;
    memset(ibuf, 0, sizeof(z_msg_ibuf_t));
    rd = __msg_parse(ibuf, pbuffer, bufsize, proto, udata);
    pbuffer += (bufsize - rd);
  }

  return(rd != 0);
}

static int __msg_read_store_pending (z_msg_ibuf_t *ibuf,
                                     const z_io_seq_vtable_t *io_proto, int *read_more,
                                     const z_msg_protocol_t *proto, void *udata)
{
  ssize_t rd;

  rd = io_proto->read(udata, ibuf->buf.blob + ibuf->available, ibuf->remaining);
  if (Z_UNLIKELY(rd < 0))
    return(-1);

  *read_more = (rd == ibuf->remaining);
  ibuf->remaining -= rd;
  ibuf->available += rd;

  if (ibuf->remaining == 0) {
    proto->publish(udata, ibuf);
  }
  return(0);
}

static int __msg_read_pending (z_msg_ibuf_t *ibuf,
                               const z_io_seq_vtable_t *io_proto, int *read_more,
                               const z_msg_protocol_t *proto, void *udata)
{
  uint8_t buffer[Z_MSG_SKIPBUF];
  size_t bufsize;
  ssize_t rd;

  do {
    bufsize = z_min(ibuf->remaining, Z_MSG_SKIPBUF);
    rd = io_proto->read(udata, buffer, bufsize);
    if (Z_UNLIKELY(rd < 0))
      return(-1);

    *read_more = (rd == bufsize);
    ibuf->remaining -= rd;
    ibuf->available += rd;
    proto->append(udata, ibuf, buffer, rd);
  } while (ibuf->remaining != 0 && rd != 0);

  if (ibuf->remaining == 0) {
    proto->publish(udata, ibuf);
  }
  return(0);
}

/* ============================================================================
 *  PUBLIC fd I/O methods
 */
void z_msg_ibuf_init (z_msg_ibuf_t *ibuf) {
  memset(ibuf, 0, sizeof(z_msg_ibuf_t));
}

int z_msg_ibuf_read (z_msg_ibuf_t *ibuf, const z_io_seq_vtable_t *io_proto,
                     const z_msg_protocol_t *proto, void *udata)
{
  int read_more;
  int r;
  do {
    if (ibuf->remaining == 0) {
      r = __msg_read_new(ibuf, io_proto, &read_more, proto, udata);
    } else if (ibuf->store) {
      r = __msg_read_store_pending(ibuf, io_proto, &read_more, proto, udata);
    } else {
      r = __msg_read_pending(ibuf, io_proto, &read_more, proto, udata);
    }
  } while (read_more && !r);
  return(r);
}
