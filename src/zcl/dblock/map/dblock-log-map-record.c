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

#include "dblock-map-private.h"

#include <zcl/int-coding.h>
#include <zcl/memutil.h>
#include <zcl/bits.h>

/*
 * +--+--+--+--+
 * |11|11|11|11|
 * +--+--+--+--+
 *  |   |  |  +---- value length
 *  |   |  +------- key length or kv-shift
 *  |   +---------- kprefix
 *  +-------------- prev offset
 */

void z_dblock_log_map_record_add (uint8_t *block,
                                  uint32_t *pnext,
                                  uint32_t *kv_last,
                                  z_dblock_kv_stats_t *stats,
                                  const z_dblock_kv_t *kv)
{
  uint32_t kprefix;
  uint8_t *rhead;
  uint8_t *pbuf;

  /* Z_ASSERT(kv->klength > 0) */

  rhead  = block + (*pnext);
  pbuf   = rhead + 1;
  *rhead = 0;

  kprefix = 0; // TODO

  if (Z_LIKELY(*kv_last != 0)) {
    uint32_t pprev = rhead - (block + *kv_last);
    const uint8_t size = z_uint32_size(pprev);
    *rhead |= size << 6;
    z_uint32_encode(pbuf, size, pprev);
    pbuf += size;
  }

  if (kprefix > 0) {
    const uint32_t size = z_uint32_size(kprefix);
    *rhead |= size << 4;
    z_uint32_encode(pbuf, size, kprefix);
    pbuf += size;
  }

  if (((kv->klength - 1) + kv->vlength) <= 66) {
    int size = 0;
    if (kv->vlength > 0) {
      size = (32 - z_clz32(kv->vlength));
      size = z_align_up(size, 2);
    }
    *rhead |= (size >> 1);
    *pbuf++ = ((kv->klength - 1) << size) | kv->vlength;
  } else {
    const uint8_t ksize = z_uint32_size(kv->klength);
    const uint8_t vsize = z_uint32_size(kv->vlength);

    *rhead |= (ksize << 2) | vsize;
    z_uint32_encode(pbuf, ksize, kv->klength); pbuf += ksize;
    z_uint32_encode(pbuf, vsize, kv->vlength); pbuf += vsize;
  }

  memcpy(pbuf, kv->key,   kv->klength); pbuf += kv->klength;
  memcpy(pbuf, kv->value, kv->vlength); pbuf += kv->vlength;

  *kv_last = (rhead - block) & 0xffffffff;
  *pnext  += (pbuf - rhead) & 0xffffffff;
  z_dblock_kv_stats_update(stats, kv);
}

const uint8_t *z_dblock_log_map_record_get (const uint8_t *block,
                                            const uint8_t *recbuf,
                                            z_dblock_kv_t *kv)
{
  uint8_t head;

  head = *recbuf;

  // skip head + pprev
  recbuf += 1 + (((head & 0xC0) >> 6) & 0x3);

  // TODO: kprefix

  if ((head & 0x0C) == 0) {
    const int size = (head & 0x3) << 1;
    kv->klength = 1 + ((*recbuf & 0xff) >> size);
    kv->vlength = (*recbuf & 0xff) & ((1 << size) - 1);
    recbuf++;
  } else {
    const uint8_t ksize = (head & 0x0C) >> 2;
    const uint8_t vsize = (head & 0x03);
    z_uint32_decode(recbuf, ksize, &(kv->klength)); recbuf += ksize;
    z_uint32_decode(recbuf, vsize, &(kv->vlength)); recbuf += vsize;
  }

  kv->key   = recbuf; recbuf += kv->klength;
  kv->value = recbuf; recbuf += kv->vlength;
  return(recbuf);
}

const uint8_t *z_dblock_log_map_record_prev (const uint8_t *block,
                                             const uint8_t *recbuf)
{
  const int size = ((*recbuf & 0xC0) >> 6) & 0x3;
  if (size > 0) {
    uint32_t pprev;
    z_uint32_decode(recbuf + 1, size, &pprev);
    return(recbuf - pprev);
  }
  return(NULL);
}

uint32_t z_dblock_log_map_kv_space (const z_dblock_kv_t *kv) {
  return Z_DBLOCK_LOG_MAP_MIN_OVERHEAD +
         z_uint32_size(kv->klength) +
         z_uint32_size(kv->vlength) +
         kv->klength + kv->vlength;
}
