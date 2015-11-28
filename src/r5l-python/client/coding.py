#!/usr/bin/env python
#
#   Copyright 2011-2013 Matteo Bertozzi
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

z_encode_zigzag = lambda n: (((n) << 1) ^ ((n) >> 63))
z_decode_zigzag = lambda n: (((n) >> 1) ^ -((n) & 1))

z_align_up = lambda x, align: ((x + (align - 1)) & (-align))

def z_fetch_bits(byte, nbits, pos):
    return (((byte) & (((1 << (nbits)) - 1) << (pos))) >> (pos))

z_fetch_7bit = lambda byte, pos: z_fetch_bits(byte, 7, pos)
z_fetch_6bit = lambda byte, pos: z_fetch_bits(byte, 6, pos)
z_fetch_5bit = lambda byte, pos: z_fetch_bits(byte, 5, pos)
z_fetch_4bit = lambda byte, pos: z_fetch_bits(byte, 4, pos)
z_fetch_3bit = lambda byte, pos: z_fetch_bits(byte, 3, pos)
z_fetch_2bit = lambda byte, pos: z_fetch_bits(byte, 2, pos)

def z_uint_bytes(value):
  if (value < (1 << 32)):
    if (value < (1 <<  8)): return(1)
    if (value < (1 << 16)): return(2)
    if (value < (1 << 24)): return(3)
    return(4)
  else:
    if (value < (1 << 40)): return(5)
    if (value < (1 << 48)): return(6)
    if (value < (1 << 56)): return(7)
    return(8)

def z_encode_uint(length, value):
  buf = bytearray(length)
  if length >= 8: buf[7] = (value >> 56) & 0xff;
  if length >= 7: buf[6] = (value >> 48) & 0xff;
  if length >= 6: buf[5] = (value >> 40) & 0xff;
  if length >= 5: buf[4] = (value >> 32) & 0xff;
  if length >= 4: buf[3] = (value >> 24) & 0xff;
  if length >= 3: buf[2] = (value >> 16) & 0xff;
  if length >= 2: buf[1] = (value >>  8) & 0xff;
  if length >= 1: buf[0] = value & 0xff;
  return buf

def z_decode_uint(buf, length, offset=0):
  result = 0;
  if length >= 8: result  = buf[offset+7] << 56
  if length >= 7: result += buf[offset+6] << 48
  if length >= 6: result += buf[offset+5] << 40
  if length >= 5: result += buf[offset+4] << 32
  if length >= 4: result += buf[offset+3] << 24
  if length >= 3: result += buf[offset+2] << 16
  if length >= 2: result += buf[offset+1] <<  8
  if length >= 1: result += buf[offset+0]
  return result

def z_encode_vint(value):
  data = bytearray()
  bits = value & 0x7f
  value >>= 7
  while value != 0:
    data.append(0x80 | bits)
    bits = value & 0x7f
    value >>= 7
  data.append(bits)
  return data

def z_decode_vint(buf):
  result = 0
  index = 0
  for shift in xrange(0, 64, 7):
    b = buf[index]
    index += 1
    if (b & 128) != 0:
      result |= (b & 0x7f) << shift
    else:
      result |= (b << shift)
      return index, result
  return -1, None

def z_uint_pack(items):
  pack_buffer = bytearray()
  count = len(items)
  index = 0
  while count > 0:
    head = 0
    vbuffer = bytearray()
    for i in xrange(min(count, 4)):
      value = items[index]
      vlength = z_align_up(z_uint_bytes(value), 2)
      head |= ((vlength >> 1) - 1) << (i << 1)
      vbuffer += z_encode_uint(vlength, value)
      index += 1
      count -= 1
    pack_buffer += chr(head) + vbuffer
  return pack_buffer

def z_uint_unpack(data, offset, length):
  items = []
  while length > 0:
    head = data[offset]
    offset += 1
    length -= 1
    for i in xrange(4):
      vlength = (1 + ((head >> (i << 1)) & 3)) << 1
      if vlength > length: break
      value = z_decode_uint(data, vlength, offset)
      items.append(value)
      offset += vlength
      length -= vlength
  return items

if __name__ == '__main__':
  for i in xrange(32):
    src = range(i + 1)
    x = z_uint_pack(src)
    dst = z_uint_unpack(x, 0, len(x))
    assert src == dst
    print src, dst
