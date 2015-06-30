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

#ifndef _Z_MSG_H_
#define _Z_MSG_H_

#include <zcl/config.h>
__Z_BEGIN_DECLS__

#include <zcl/macros.h>
#include <zcl/io.h>

typedef struct z_msg_protocol z_msg_protocol_t;
typedef struct z_msg_ibuf z_msg_ibuf_t;
typedef struct z_msg_obuf z_msg_obuf_t;

enum z_msg_ibuf_action {
  Z_MSG_IBUF_STORE,
  Z_MSG_IBUF_DISCARD,
  Z_MSG_IBUF_USER,
};

struct z_msg_ibuf {
  uint32_t remaining;
  uint32_t available;
  uint32_t fwd_length;

  uint8_t  buflen;
  uint8_t  store;
  uint8_t  pkg_type;
  uint8_t  framelen;

  /* rdcount, first_rd_usec */

  union msg_ibuf {
    uint8_t *blob;
    uint8_t  frame[8];
  } buf;
};

struct z_msg_obuf {
};

struct z_msg_protocol {
  int   (*alloc)    (void *udata, z_msg_ibuf_t *ibuf, uint32_t length);
  int   (*append)   (void *udata, z_msg_ibuf_t *ibuf,
                     const uint8_t *buffer, uint32_t size);
  int   (*publish)  (void *udata, z_msg_ibuf_t *ibuf);
};

void z_msg_ibuf_init (z_msg_ibuf_t *ibuf);

int z_msg_ibuf_read (z_msg_ibuf_t *ibuf, const z_io_seq_vtable_t *io_proto,
                     const z_msg_protocol_t *proto, void *udata);

__Z_END_DECLS__

#endif /* !_Z_MSG_H_ */
