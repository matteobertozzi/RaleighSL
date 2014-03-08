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

#include <zcl/bytes-packing.h>
#include <zcl/int-coding.h>
#include <zcl/debug.h>

/*
 * 8 7 6  4 3 2  0
 * +-+-+--+-+-+--+ +---------------+ +---------------+ +------------+ +--------+
 * |A|A|AA|B|B|BB| | A prefix vlen | | A suffix vlen | | A data len | | A data |
 * +-+-+--+-+-+--+ +---------------+ +---------------+ +------------+ +--------+
 *  | | |
 *  | | |------> data width (0 same length as before, 1-3 bytes)
 *  | |--------> has suffix
 *  |----------> has prefix
 */

/* ============================================================================
 *  PUBLIC unpacker methods
 */
void z_bytes_unpacker_open (z_bytes_unpacker_t *self,
                            uint8_t *last_item,
                            const uint8_t *buffer,
                            uint32_t size)
{
  self->nitems = 0;
  self->last_bsize = 0;
  self->last_size = 0;
  self->last_item = last_item;
  self->head = 0;

  self->buf_avail = size;
  self->pbuffer = buffer;
}

static const uint8_t *__bytes_unpacker_fetch (z_bytes_unpacker_t *self,
                                              uint32_t *prefix,
                                              uint32_t *suffix,
                                              uint32_t *bsize)
{
  const uint8_t *ibuf;
  uint8_t head;

  ibuf = self->pbuffer;
  if (++self->nitems & 1) {
    self->head = *ibuf++;
    head = self->head >> 4;
  } else {
    head = self->head & 0xf;
  }

  if (head >> 3) {
    uint8_t vsize;
    vsize = z_vint32_decode(ibuf, prefix);
    ibuf += vsize;
  } else {
    *prefix = 0;
  }

  if ((head >> 2) & 1) {
    uint8_t vsize;
    vsize = z_vint32_decode(ibuf, suffix);
    ibuf += vsize;
  } else {
    *suffix = 0;
  }

  if (head & 3) {
    uint8_t bwidth = (head & 3);
    z_uint32_decode(ibuf, bwidth, bsize);
    ibuf += bwidth;
  } else {
    *bsize = self->last_bsize;
  }

  return(ibuf);
}

static void __bytes_unpacker_copy_body (z_bytes_unpacker_t *self,
                                        const uint8_t *ibuf,
                                        uint32_t prefix,
                                        uint32_t suffix,
                                        uint32_t bsize)
{
  if (suffix > 0) {
    /* TODO: use last_bsize to avoid this move if they are the same */
    uint8_t *psuffix = self->last_item + self->last_size - suffix;
    memmove(self->last_item + prefix + bsize, psuffix, suffix);
  }

  memcpy(self->last_item + prefix, ibuf, bsize);
  ibuf += bsize;

  self->last_bsize = bsize;
  self->last_size = prefix + bsize + suffix;
  self->buf_avail -= (ibuf - self->pbuffer);
  self->pbuffer = ibuf;
}

int z_bytes_unpacker_next (z_bytes_unpacker_t *self) {
  const uint8_t *ibuf;
  uint32_t prefix;
  uint32_t suffix;
  uint32_t bsize;

  if (Z_UNLIKELY(self->buf_avail == 0))
    return(0);

  ibuf = __bytes_unpacker_fetch(self, &prefix, &suffix, &bsize);
  __bytes_unpacker_copy_body(self, ibuf, prefix, suffix, bsize);
  return(self->buf_avail > 0);
}

int z_bytes_unpacker_ssearch (z_bytes_unpacker_t *self,
                              const void *key,
                              uint32_t ksize,
                              uint32_t *index)
{
  const uint8_t *pkey = (const uint8_t *)key;
  uint32_t koffset = 0;

  while (self->buf_avail > 0) {
    const uint8_t *ibuf;
    uint32_t prefix;
    uint32_t suffix;
    uint32_t bsize;
    size_t kshared;

    ibuf = __bytes_unpacker_fetch(self, &prefix, &suffix, &bsize);
    __bytes_unpacker_copy_body(self, ibuf, prefix, suffix, bsize);

    if (koffset > prefix) {
      *index = self->nitems;
      return(1);
    }

    ibuf = self->last_item + koffset;
    bsize = self->last_size - koffset;
    kshared = z_memshared(ibuf, bsize, pkey, ksize);
    if (kshared == ksize && bsize == ksize) {
      *index = self->nitems;
      return(0); /* is delete marker? */
    }

    if (ibuf[kshared] > pkey[kshared]) {
      *index = self->nitems;
      return(-1);
    }

    koffset += kshared;
    ksize -= kshared;
    pkey += kshared;
  }

  *index = self->nitems;
  return(1);
}

int z_bytes_unpacker_seek (z_bytes_unpacker_t *self, int index) {
  while (self->nitems < index && self->buf_avail > 0) {
    const uint8_t *ibuf;
    uint32_t prefix;
    uint32_t suffix;
    uint32_t bsize;

    /* TODO: We can avoid copying stuff if the next prefix and suffix are 0 */
    ibuf = __bytes_unpacker_fetch(self, &prefix, &suffix, &bsize);
    __bytes_unpacker_copy_body(self, ibuf, prefix, suffix, bsize);
  }

  return(self->nitems);
}

/* ============================================================================
 *  PUBLIC packer methods
 */
void z_bytes_packer_open (z_bytes_packer_t *self) {
  self->nitems = 0;
  self->last_bsize = 0;
  self->last_size = 0;
  self->last_item = NULL;
  self->head = NULL;
}

uint8_t *z_bytes_packer_close (z_bytes_packer_t *self, uint8_t *obuf) {
  return(obuf);
}

uint8_t *z_bytes_packer_add (z_bytes_packer_t *self,
                             const void *data,
                             uint32_t size,
                             uint8_t *obuf)
{
  const uint8_t *body = data;
  uint32_t prefix, suffix;
  uint32_t bsize = size;
  uint8_t head;

  prefix = z_memshared(self->last_item, self->last_size, body, bsize);
  bsize -= prefix;
  body += prefix;

  suffix = z_memrshared(self->last_item, self->last_size, body, bsize);
  bsize -= suffix;

  Z_ASSERT(bsize < (1ul << 24), "16M string should be stored somewhere else");
  head = 0;

  if (++self->nitems & 1) {
    *obuf = 0;
    self->head = obuf++;
  }

  if (prefix != 0) {
    head |= (1 << 3);
    obuf = z_vint32_encode(obuf, prefix);
  }

  if (suffix != 0) {
    head |= (1 << 2);
    obuf = z_vint32_encode(obuf, suffix);
  }

  if (bsize != self->last_bsize) {
    unsigned int bwidth = z_uint32_size(bsize);
    head |= bwidth;
    self->last_bsize = bsize;
    z_uint32_encode(obuf, bwidth, bsize);
    obuf += bwidth;
  }

  if (self->nitems & 1) {
    *(self->head) = (head << 4);
  } else {
    *(self->head) |= head;
  }

  // TODO: if prefix + suffix is 0, we can avoid copy
  fprintf(stderr, "prefix=%u suffix=%u bsize=%u size=%u %s %s\n",
                  prefix, suffix, bsize, size, data, self->last_item);
  memcpy(self->last_item + prefix, ((const uint8_t *)data) + prefix, size - prefix);

  self->last_size = size;
  memcpy(obuf, body, bsize);
  return(obuf + bsize);
}
