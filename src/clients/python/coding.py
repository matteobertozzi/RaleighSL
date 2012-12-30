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

def z_decode_uint(buf, length):
  result = 0;
  if length >= 8: result  = buf[7] << 56
  if length >= 7: result += buf[6] << 48
  if length >= 6: result += buf[5] << 40
  if length >= 5: result += buf[4] << 32
  if length >= 4: result += buf[3] << 24
  if length >= 3: result += buf[2] << 16
  if length >= 2: result += buf[1] <<  8
  if length >= 1: result += buf[0]
  return result


def z_encode_field(field_id, length):
  buf = bytearray(1)

  assert length > 0, "Length must be greater then 0!"
  if (length <= 8):
    # Internal Length (uint8-uint64) */
    buf[0] = ((length - 1) & 0x7) << 4
  else:
    # External Length
    flen = z_uint_bytes(length)
    buf[0] = (1 << 7) | (((flen - 1) & 0x7) << 4)
    buf.extend(z_encode_uint(flen, length))

  if (field_id <= 13):
      # Field-Id can stay in the head
      buf[0] |= field_id + 2
  else:
    # Field-Id is to high, store Field-Id length here (max 2 byte)
    flen = z_uint_bytes(field_id);
    buf[0] |= (flen - 1);
    buf.extend(z_encode_uint(flen, field_id))

  return buf

def z_decode_field(buf):
    elength = 1;

    flen = 1 + z_fetch_3bit(buf[0], 4)
    if (buf[0] & (1 << 7)):
        # External Length
        elength += flen;
        if (len(buf) < elength):
          return -1, None, None
        length = z_decode_uint(buf[1:], flen)
    else:
        # Internal Length (uint8-uint64)
        length = flen

    flen = z_fetch_4bit(buf[0], 0);
    if (flen >= 2):
        # Field-Id is in the head
        field_id = flen - 2;
    else:
        # Field-Id is stored outside
        flen += 1;
        if (len(buf) < (elength + flen)):
          return -1, None, None
        field_id = z_decode_uint(buf[elength:], flen)
        elength += flen

    return elength, field_id, length

if __name__ == '__main__':
  for i in xrange(100):
    buf = z_encode_field(i, 1 + i * 2)
    elen, field_id, length = z_decode_field(buf)
    assert len(buf) == elen, (len(buf), elen)
    assert field_id == i, (field_id, i)
    assert length == (1 + i * 2), (length, (1 + i * 2))