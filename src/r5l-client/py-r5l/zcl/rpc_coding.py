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

from io import BytesIO
from coding import *
import re

class FieldReader(object):
  def __init__(self, fields, body, blob):
    self.fields = fields
    self._kvs = {}
    self.body = body
    self.blob = blob

  def keys(self):
    return self._kvs.iterkeys()

  def __repr__(self):
    return str(self._kvs)

  def __getattr__(self, name):
    value = self._kvs.get(name)
    if value is None:
      raise AttributeError(name)
    return value

  def parse(self):
    UNKNOWN_FIELD = (None, None, None)
    kvs = {field_name: field_default
            for field_id, (field_name, field_type, field_default) in self.fields.iteritems()
              if not field_default is None}

    offset = 1
    nfields = int(self.body[0])
    data_fields = []
    blob_fields = []
    for i in xrange(nfields):
      offset, field_id, field_length = self._read_field(offset)
      field_name, field_type, _ = self.fields.get(field_id, UNKNOWN_FIELD)
      if field_type is None:
        pass
      elif isinstance(field_type, dict) or field_type.startswith('list') or field_type in ('string', 'bytes'):
        vlength = self._read_field_uint(field_length, offset)
        data_fields.append((field_name, field_type, vlength))
      elif field_type == 'blob':
        vlength = self._read_field_uint(field_length, offset)
        blob_fields.append((field_name, field_type, vlength))
      else:
        func = getattr(self, '_read_field_' + field_type, None)
        kvs[field_name] = func(field_length, offset)
      offset += field_length

    # Parse Body Data
    for field_name, field_type, field_length in data_fields:
      if isinstance(field_type, dict):
        ireader = FieldReader(field_type, self.body[offset:], None)
        kvs[field_name] = ireader.parse()
      elif field_type.startswith('list'):
        field_type = re.match('list\[(\w+)\]', field_type).groups()[0]
        func = getattr(self, '_read_list_' + field_type, None)
        kvs[field_name] = func(field_length, offset)
      elif field_type == 'string':
        kvs[field_name] = self._read_string(field_length, offset)
      elif field_type == 'bytes':
        kvs[field_name] = self._read_bytes(field_length, offset)
      offset += field_length

    # Parse Blob
    offset = 0
    for field_name, field_type, field_length in blob_fields:
      kvs[field_name] = self._read_blob(field_length, offset)
      offset += field_length

    self._kvs = kvs
    return kvs

  def _read_field(self, offset):
    block = self.body
    field_length = block[offset] & 7
    field_id = block[offset] >> 3
    offset += 1
    if field_id == 30:
      field_id += block[offset]
      offset += 1
    elif field_id == 31:
      field_id += block[offset] << 8
      offset += 1
      field_id += block[offset]
      offset += 1
    return offset, field_id, field_length + 1

  def _read_field_uint(self, vlength, offset):
    return z_decode_uint(self.body[offset:offset+vlength], vlength)

  def _read_field_int(self, vlength, offset):
    value = z_decode_uint(self.body[offset:offset+vlength], vlength)
    return z_decode_zigzag(value)

  def _read_list_uint(self, vlength, offset):
    data = z_uint_unpack(self.body, offset, vlength)
    return data

  def _read_list_int(self, vlength, offset):
    items = self._read_list_uint(vlength, offset)
    return [z_decode_zigzag(v) for v in items]

  def _read_string(self, vlength, offset):
    return self.body[offset:offset+vlength-1]

  def _read_bytes(self, vlength, offset):
    return self.body[offset:offset+vlength]

  def _read_blob(self, vlength, offset):
    return self.blob[offset:vlength]

class FieldWriter(object):
  def __init__(self):
    self._body_buffer = BytesIO()
    self._body_data = None
    self._blob_data = None
    self._nfields = 0

  def get_body(self):
    body = self._body_buffer.getvalue()
    if self._body_data:
      data = self._body_data.getvalue()
      return bytearray(chr(self._nfields) + body + data)
    return bytearray(chr(self._nfields) + body)

  def get_blob(self):
    if self._blob_data:
      return self._blob_data.getvalue()
    return ''

  # =================================================================
  #  UInt/Int Fields
  # =================================================================
  def write_uint(self, field_id, value):
    assert value < (1 << 64)
    vlength = z_uint_bytes(value)
    self._write_field(field_id, vlength - 1)
    self._body_buffer.write(z_encode_uint(vlength, value))

  def write_int(self, field_id, value):
    self.write_uint(self, field_id, z_encode_zigzag(value))

  # =================================================================
  #  String/Byte/Blob Fields
  # =================================================================
  def write_string(self, field_id, value):
    self.write_uint(field_id, len(value) + 1)
    body_write = self._get_body_data_writer()
    body_write(value)
    body_write('\x00')

  def write_bytes(self, field_id, value):
    self.write_uint(field_id, len(value))
    body_write = self._get_body_data_writer()
    body_write(value)

  def write_blob(self, field_id, value):
    self.write_uint(field_id, len(value))
    blob_write = self._get_blob_writer()
    blob_write(value)

  def write_struct(self, field_id, value):
    self.write_uint(field_id, len(value))
    body_write = self._get_body_data_writer()
    body_write(value)

  # =================================================================
  #  List Fields
  # =================================================================
  def write_list_uint(self, field_id, items):
    body_write = self._get_body_data_writer()
    data = z_uint_pack(items)
    body_write(data)
    self.write_uint(field_id, len(data))

  def write_list_int(self, field_id, items):
    items = [z_encode_zigzag(v) for v in items]
    self.write_list_uint(field_id, items)

  def write_list_string(self, field_id, items):
    self.write_uint(field_id, len(items))
    body_write = self._get_body_data_writer()
    for value in items:
      body_write(z_encode_vint(len(value) + 1))
      body_write(value)
      body_write('\x00')

  def write_list_bytes(self, field_id, items):
    self.write_uint(field_id, len(items))
    body_write = self._get_body_data_writer()
    for value in items:
      body_write(z_encode_vint(len(value)))
      body_write(value)

  def write_list_blob(self, field_id, items):
    self.write_uint(field_id, len(items))
    blob_write = self._get_blob_writer()
    for value in items:
      blob_write(z_encode_vint(len(value)))
      blob_write(value)

  # =================================================================
  #  PRIVATE Helpers
  # =================================================================
  def _write_field(self, field_id, field_length):
    assert field_length < 8
    assert field_id < (1 << 16)
    if field_id < 30:
      buf = bytearray(1)
      buf[0] = (field_id << 3) | field_length
    elif field_id > 285:
      field_id -= 31
      buf = bytearray(3)
      buf[0] = (31 << 3) | field_length
      buf[1] = field_id >> 8
      buf[2] = field_id & 0xff
    else:
      field_id -= 30
      buf = bytearray(2)
      buf[0] = (30 << 3) | field_length
      buf[1] = field_id & 0xff
    self._body_buffer.write(buf)
    self._nfields += 1

  def _get_body_data_writer(self):
    if self._body_data:
      return self._body_data.write
    blob = BytesIO()
    blob_write = blob.write
    self._body_data = blob
    return blob_write

  def _get_blob_writer(self):
    if self._blob_data:
      return self._blob_data.write
    blob = BytesIO()
    blob_write = blob.write
    self._blob_data = blob
    return blob_write

if __name__ == '__main__':
  from time import time

  writer = FieldWriter()
  for i in xrange(10):
    writer.write_uint(i, i * 10)
  writer.write_list_uint(11, [1, 2, 3, 4, 5, 6])
  writer.write_string(12, 'Hello World')
  writer.write_bytes(13, 'Puppa World')
  writer.write_struct(14, writer.get_body())
  writer.write_blob(15, 'BLOBBA')

  fields = {}
  for i in xrange(10):
    fields[i] = ('%02d' % i, 'uint', None)
  fields[11] = ('11', 'list[uint]', None)
  fields[12] = ('12', 'string', None)
  fields[13] = ('13', 'bytes', None)
  fields[14] = ('14', dict(fields), None)
  fields[15] = ('15', 'blob', None)

  data = FieldReader(fields, writer.get_body(), writer.get_blob())
  kvs = data.parse()
  for k, v in sorted(kvs.iteritems()):
    print k, v
