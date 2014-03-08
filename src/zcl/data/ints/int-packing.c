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

#include <zcl/int-packing.h>
#include <zcl/int-coding.h>

/*
 * 8 7   4 3   0
 * +-+---+-+---+ +--------+-------+-------+-------+-------+
 * |1|111|1|111| | widths | count | delta | count | delta |
 * +-+---+-+---+ +--------+-------+-------+-------+-------+
 *  | |   | |----- ndeltas
 *  | |   +------- singles
 *  | +----------- width
 *  +------------- fix-w
 *
 * u16 widths head
 * +-+-+-+-+-+-+-+-+
 * |1|1|1|1|1|1|1|1|
 * +-+-+-+-+-+-+-+-+
 * 8 7 6 5 4 3 2 1 0
 *
 * u32 widths head
 * +--+--+--+--+ +--+--+--+--+
 * |11|11|11|11| |11|11|11|11|
 * +--+--+--+--+ +--+--+--+--+
 * 8  6  4  2  0 8  6  4  2  0
 *
 * u64 widths head
 * +---+---+--+ +-+---+---+-+ +--+---+---+
 * |111|111|11| |1|111|111|1| |11|111|111|
 * +---+---+--+ +-+---+---+-+ +--+---+---+
 * 8   5   2  0 8 7   4   1 0 8  6   3   0
 */

/* ============================================================================
 *  PRIVATE unpacker methods
 */
static void __uint_unpacker_fetch (z_uint_unpacker_t *self) {
  const uint8_t width_map[9] = { 0, 0, 1, 2, 2, 3, 3, 3, 3 };
  uint8_t width;

  //Z_LOG_TRACE("unpack_next windex=%d ndeltas=%d buf_avail=%d\n",
  //                self->windex, self->ndeltas, self->buf_avail);
  if (self->windex >= self->ndeltas) {
    uint8_t head;

    /*
     * 8 7   4 3   0
     * +-+---+-+---+
     * |1|111|1|111|
     * +-+---+-+---+
     *  | |   | |----- ndeltas
     *  | |   +------- singles
     *  | +----------- width
     *  +------------- fix-w
     */
    --self->buf_avail;
    head  = *self->pbuffer++;
    self->fixw    = (head & 0x80) >> 7;
    self->width   = 1 + ((head & 0x70) >> 4);
    self->singles = (head & 0x08) >> 3;
    self->ndeltas = 1 + (head & 0x07);

    self->windex = 1;
    self->whead[0] = 0;
    self->whead[1] = 0;
    self->whead[2] = 0;

    if (self->fixw) {
      width = self->width;
    } else {
      unsigned int whead_size;
      switch (width_map[self->width]) {
        case 0:
          width = self->width;
          whead_size = 0;
          break;
        case 1:
          whead_size = 1 + ((self->ndeltas - 1) >> 1);
          memcpy(self->whead, self->pbuffer, whead_size);
          width = 1 + (self->whead[0] & 0x1);
          break;
        case 2:
          whead_size = 1 + ((self->ndeltas - 1) >> 2);
          memcpy(self->whead, self->pbuffer, whead_size);
          width = 1 + (self->whead[0] & 0x3);
          break;
        case 3:
          whead_size = z_align_up(self->ndeltas * 3, 8) >> 3;
          memcpy(self->whead, self->pbuffer, whead_size);
          width = 1 + (self->whead[0] >> 5);
          break;
      }
      self->buf_avail -= whead_size;
      self->pbuffer += whead_size;
    }

    //Z_LOG_TRACE("unpack head ndeltas=%d fixw=%d width=%d singles=%d\n",
    //                self->ndeltas, self->fixw, self->width, self->singles);
  } else {
    if (self->fixw) {
      width = self->width;
      ++self->windex;
    } else {
      unsigned int i = self->windex++;
      switch (width_map[self->width]) {
        case 0: {
          width = self->width;
          break;
        }
        case 1: {
          /*
           * +-+-+-+-+-+-+-+-+
           * |1|1|1|1|1|1|1|1|
           * +-+-+-+-+-+-+-+-+
           * 8 7 6 5 4 3 2 1 0
           */
          width = 1 + ((self->whead[i >> 3] >> (i & 7)) & 0x1);
          break;
        } case 2: {
          /*
           * +--+--+--+--+ +--+--+--+--+
           * |11|11|11|11| |11|11|11|11|
           * +--+--+--+--+ +--+--+--+--+
           * 8  6  4  2  0 8  6  4  2  0
           */
          width = 1 + ((self->whead[i >> 2] >> ((i & 3) << 1)) & 0x3);
          break;
        } case 3: {
          /*
           * +---+---+--+ +-+---+---+-+ +--+---+---+
           * |111|111|11| |1|111|111|1| |11|111|111|
           * +---+---+--+ +-+---+---+-+ +--+---+---+
           * 8   5   2  0 8 7   4   1 0 8  6   3   0
           */
          switch (i) {
            case 0: width = 1 + ((self->whead[0] >> 5) & 0x07); break;
            case 1: width = 1 + ((self->whead[0] >> 2) & 0x07); break;
            case 2: width = 1 + (((self->whead[0] & 0x3) << 1) | (self->whead[1] >> 7)); break;
            case 3: width = 1 + ((self->whead[1] >> 4) & 0x07); break;
            case 4: width = 1 + ((self->whead[1] >> 1) & 0x07); break;
            case 5: width = 1 + (((self->whead[1] & 0x1) << 2) | (self->whead[2] >> 6)); break;
            case 6: width = 1 + ((self->whead[2] >> 3) & 0x07); break;
            case 7: width = 1 + ((self->whead[2] & 0x07)); break;
          }
          break;
        }
      }
    }
  }

  /* fetch the repetition count */
  if (self->singles) {
    self->count = 0;
  } else {
    --self->buf_avail;
    self->count = *self->pbuffer++;
  }

  /* fetch the first delta */
  self->delta = 0;
  memcpy(&(self->delta), self->pbuffer, width);
  self->buf_avail -= width;
  self->pbuffer += width;
}

#define __uint_unpacker_has_next(self)        \
  ((self)->buf_avail != 0 ||                  \
    (self)->count > 0 ||                      \
   (self)->windex < (self)->ndeltas)

/* ============================================================================
 *  PUBLIC unpacker methods
 */
void z_uint_unpacker_open (z_uint_unpacker_t *self, const uint8_t *buffer, uint32_t size) {
  self->value = 0;
  self->delta = 0;

  self->count = 0;
  self->windex = 0;

  self->fixw = 0;
  self->width = 0;
  self->singles = 0;
  self->ndeltas = 0;

  self->buf_avail = size;
  self->pbuffer = buffer;
}

int z_uint_unpacker_next (z_uint_unpacker_t *self) {
  //Z_LOG_TRACE("unpack_next count=%d\n", self->count);
  if (self->count > 0) {
    --self->count;
  } else {
    if (Z_UNLIKELY(self->buf_avail == 0))
      return(0);

    __uint_unpacker_fetch(self);
  }
  /* update the value */
  self->value += self->delta;

  //Z_LOG_TRACE("count=%d windex=%d ndeltas=%d buf_avail=%d\n", self->count, self->windex, self->ndeltas, self->buf_avail);
  return(__uint_unpacker_has_next(self));
}
#include <stdio.h>
int z_uint_unpacker_ssearch (z_uint_unpacker_t *self, uint64_t key, uint32_t *index) {
  uint64_t kindex = 0;
  uint64_t kdelta;

  if (Z_UNLIKELY(self->count > 0)) {
    if (key < self->value)
      return(-1);

    kdelta = self->value + (self->delta * self->count);
    if (key > kdelta) {
      self->value = kdelta;
      kindex += self->count;
      self->count = 0;
    } else {
      kdelta = key - self->value;
      *index = kindex + (kdelta / self->delta);
      return((kdelta % self->delta) != 0 ? -1 : 0);
    }
  }

  while (__uint_unpacker_has_next(self)) {
    __uint_unpacker_fetch(self);
    self->value += self->delta;

    kdelta = self->value + (self->delta * self->count);
    if (key > kdelta) {
      self->value = kdelta;
      kindex += self->count;
      self->count = 0;
      continue;
    }

    if (key < self->value)
      return(-1);

    kdelta = key - self->value;
    *index = kindex + (kdelta / self->delta);
    return((kdelta % self->delta) != 0 ? -1 : 0);
  }
  return(1);
}

int z_uint_unpacker_seek (z_uint_unpacker_t *self, int index) {
  int sindex = 0;

  if (Z_UNLIKELY(self->count > 0)) {
    if (self->count >= index) {
      self->count -= index;
      return(index);
    }
    sindex = self->count;
    index -= self->count;
    self->count = 0;
  }

  while (__uint_unpacker_has_next(self)) {
    int count;

    __uint_unpacker_fetch(self);
    self->value += self->delta;
    count = 1 + self->count;

    if (count >= index) {
      self->count -= index;
      return(sindex + index);
    }

    index -= count;
    sindex += count;
    self->count = 0;
  }

  return(sindex);
}

/* ============================================================================
 *  PRIVATE packer methods
 */
static uint8_t *__uint_packer_flush_fix (z_uint_packer_t *self,
                                         const uint8_t singles,
                                         uint8_t *obuf)
{
  unsigned int ndeltas = self->ndeltas;
  const uint8_t *ibuf = self->buffer;

  while (ndeltas--) {
    int cpy_size = !singles + ibuf[0];
    //Z_LOG_TRACE("fix %d count %d\n", (int)ibuf[0], (int)ibuf[1]);
    memcpy(obuf, ibuf + 1 + singles, cpy_size);
    obuf += cpy_size;
    ibuf += 2 + ibuf[0];
  }

  return(obuf);
}

static uint8_t *__uint_packer_flush_u16 (z_uint_packer_t *self,
                                         const uint8_t singles,
                                         uint8_t *obuf)
{
  unsigned int ndeltas = self->ndeltas;
  const uint8_t *ibuf = self->buffer;
  uint8_t *head;
  int i;

  head = obuf;
  head[0] = 0;
  head[1] = 0;

  obuf += 1 + ((ndeltas - 1) >> 1);
  for (i = 0; i < ndeltas; ++i) {
    //Z_LOG_TRACE("u16 WRITE %u count %u\n", ibuf[0], ibuf[1]);
    int cpy_size = !singles + ibuf[0];
    head[i >> 3] |= ((ibuf[0] - 1) << (i & 7));
    memcpy(obuf, ibuf + 1 + singles, cpy_size);
    obuf += cpy_size;
    ibuf += 2 + ibuf[0];
  }

  return(obuf);
}

static uint8_t *__uint_packer_flush_u32 (z_uint_packer_t *self,
                                         const uint8_t singles,
                                         uint8_t *obuf)
{
  unsigned int ndeltas = self->ndeltas;
  const uint8_t *ibuf = self->buffer;
  uint8_t *head;
  int i;

  head = obuf;
  head[0] = 0; head[1] = 0;
  head[2] = 0; head[3] = 0;

  obuf += 1 + ((ndeltas - 1) >> 2);
  for (i = 0; i < ndeltas; ++i) {
    //Z_LOG_TRACE("u32 WRITE %u count %u\n", ibuf[0], ibuf[1]);
    int cpy_size = !singles + ibuf[0];
    head[i >> 2] |= ((ibuf[0] - 1) << ((i & 3) << 1));
    memcpy(obuf, ibuf + 1 + singles, cpy_size);
    obuf += cpy_size;
    ibuf += 2 + ibuf[0];
  }

  return(obuf);
}

static uint8_t *__uint_packer_flush_u64 (z_uint_packer_t *self,
                                         const uint8_t singles,
                                         uint8_t *obuf)
{
  unsigned int ndeltas = self->ndeltas;
  const uint8_t *ibuf = self->buffer;
  uint8_t hidx = 0;
  uint8_t *head;

  head = obuf;
  head[0] = 0;
  head[1] = 0;
  head[2] = 0;

  /*
   * +---+---+--+ +-+---+---+-+ +--+---+---+
   * |111|111|11| |1|111|111|1| |11|111|111|
   * +---+---+--+ +-+---+---+-+ +--+---+---+
   * 8   5   2  0 8 7   4   1 0 8  6   3   0
   */
  obuf += (z_align_up(self->ndeltas * 3, 8) >> 3);
  while (ndeltas--) {
    int cpy_size = !singles + ibuf[0];

    int hlen = ibuf[0] - 1;
    switch (hidx++) {
      case 0:
        head[0] |= (hlen << 5);
        break;
      case 1:
        head[0] |= (hlen << 2);
        break;
      case 2:
        head[0] |= (hlen & 0x6) >> 1;
        head[1] |= (hlen & 1) << 7;
        break;
      case 3:
        head[1] |= (hlen << 4);
        break;
      case 4:
        head[1] |= (hlen << 1);
        break;
      case 5:
        head[1] |= (hlen & 0x4) >> 2;
        head[2] |= (hlen & 0x3) << 6;
        break;
      case 6:
        head[2] |= (hlen << 3);
        break;
      case 7:
        head[2] |= hlen;
        break;
    }

    //Z_LOG_TRACE("u64 WRITE %u count %u\n", ibuf[0], ibuf[1]);
    memcpy(obuf, ibuf + 1 + singles, cpy_size);
    obuf += cpy_size;
    ibuf += 2 + ibuf[0];
  }

  return(obuf);
}

static uint8_t *__uint_packer_flush (z_uint_packer_t *self, uint8_t *obuf) {
  uint8_t fixw, singles;
  uint8_t *obuf_begin;

  fixw = (self->min_width == self->max_width);
  singles = self->singles;

  obuf_begin = obuf;
  *obuf++ = (fixw << 7) | ((self->max_width - 1) << 4) | (singles << 3) | (self->ndeltas - 1);

  if (fixw) {
    obuf = __uint_packer_flush_fix(self, singles, obuf);
  } else {
    const uint8_t width_map[9] = { 0, 0, 1, 2, 2, 3, 3, 3, 3 };
    switch (width_map[self->max_width]) {
      case 0: obuf = __uint_packer_flush_fix(self, singles, obuf); break;
      case 1: obuf = __uint_packer_flush_u16(self, singles, obuf); break;
      case 2: obuf = __uint_packer_flush_u32(self, singles, obuf); break;
      case 3: obuf = __uint_packer_flush_u64(self, singles, obuf); break;
    }
  }

  //Z_LOG_TRACE("FLUSH %-4d ndeltas=%d fixw=%d width=%d singles=%d buf_size=%-4lu\n", *obuf_begin,
  //            self->ndeltas, fixw, self->max_width, singles, obuffer->size);

  self->ndeltas = 0;
  self->singles = 1;

  self->min_width = self->width;
  self->max_width = self->width;

  self->buf_used = 0;
  //Z_LOG_TRACE("FLUSH last_value=%llu last_delta=%llu width=%u\n",
  //                self->last_value, self->last_delta, self->width);
  return(obuf);
}

static void __uint_packer_add_block (z_uint_packer_t *self) {
  uint8_t *pbuf = self->buffer + self->buf_used;
  *pbuf++ = self->width;
  *pbuf++ = self->count;
  z_uint_encode(pbuf, self->width, self->last_delta);
  self->buf_used += 2 + self->width;
  self->singles &= (self->count == 0);
  ++self->ndeltas;
  //printf("ADD BLOCK width=%d count=%d delta=%llu\n", self->width, self->count, self->last_delta);
}

/* ============================================================================
 *  PUBLIC packer methods
 */
void z_uint_packer_open (z_uint_packer_t *self, uint64_t value) {
  self->last_value = value;
  self->last_delta = value;

  self->count = 0;
  self->ndeltas = 0;
  self->singles = 1;

  self->width = z_uint64_size(value);
  self->min_width = self->width;
  self->max_width = self->width;

  self->buf_used = 0;
}

uint8_t *z_uint_packer_close (z_uint_packer_t *self, uint8_t *obuf) {
  if (self->buf_used > 0) {
    if (((39 - self->buf_used) < self->width) || self->ndeltas == 8)
      obuf = __uint_packer_flush(self, obuf);

    __uint_packer_add_block(self);
    obuf = __uint_packer_flush(self, obuf);
  }
  return(obuf);
}

uint8_t *z_uint_packer_add (z_uint_packer_t *self, uint64_t value, uint8_t *obuf) {
  uint64_t delta;

  delta = value - self->last_value;
  self->last_value = value;
  //Z_LOG_TRACE("ADD VALUE %llu delta %llu count %u\n", value, delta, self->count);
  // TODO: NDELTAS <= 8
  if (delta == self->last_delta && self->count < 0xff && self->ndeltas <= 8) {
    ++self->count;
  } else {
    uint8_t delta_width = z_uint64_size(delta);

    /* flush if avail (41 - self->buf_used) < (2 + self->width) */
    if (((39 - self->buf_used) < self->width) || self->ndeltas == 8 ||
        ((delta_width > self->max_width) &&
         (self->max_width <= 5 && self->ndeltas > 4)))
    {
      obuf = __uint_packer_flush(self, obuf);
    }

    /* Add block for the last delta */
    __uint_packer_add_block(self);

    /* Prepare new block */
    self->last_delta = delta;
    self->count = 0;

    self->width = delta_width;
    self->max_width = z_max(self->max_width, delta_width);
    self->min_width = z_min(self->min_width, delta_width);
  }
  return(obuf);
}
