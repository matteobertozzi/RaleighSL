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

# =================================================================
# NTOE: DO NOT USE THIS VERSION ON THE OFFICIAL CODE BASE
#       (this is not the compiler used for to build the C client)
# =================================================================

import sys
import re
import os

# =============================================================================
#  Util
# =============================================================================
align_up = lambda x, align: ((x + (align - 1)) & (-align))
ceil = lambda a, b: ((a + b - 1) / b)
spacing = lambda space, data: ' ' * (space - len(data))

def replace(text, *args):
  for items in args:
    for key, value in items.iteritems():
      text = text.replace(key, str(value))
  return text.rstrip()

# =============================================================================
#  code generator
# =============================================================================
class CodeGenerator(object):
  def __init__(self, module):
    self.module = module

  def add(self, entity):
    func = getattr(self, 'add_%s' % entity.etype)
    return func(entity)

  def add_struct(self, entity):
    raise NotImplementedError

  def add_rpc(self, entity):
    raise NotImplementedError

  def write(self, outdir):
    raise NotImplementedError

  def write_blob(self, fd, rvars, blob):
    fd.write(replace(blob, rvars))

# =============================================================================
#  C code generator
# =============================================================================
class CCodeGenerator(CodeGenerator):
  C_PRIMITIVE_TYPES_MAP = {
    'bool': 'int8_t',
    'int8': 'int8_t', 'int16': 'int16_t', 'int32': 'int32_t', 'int64': 'int64_t',
    'uint8': 'uint8_t', 'uint16': 'uint16_t', 'uint32': 'uint32_t', 'uint64': 'uint64_t',
    'bytes': 'z_bytes_t',
  }

  C_PRIMITIVE_TYPES_NAME_MAP = {'bool': 'int8'}

  def __init__(self, module):
    super(CCodeGenerator, self).__init__(module)
    self.headers = []
    self.headers_client = []
    self.headers_server = []
    self.sources = []
    self.sources_client = []
    self.sources_server = []

  def _get_def_type(self, field):
    ctype = self.C_PRIMITIVE_TYPES_MAP.get(field.vtype, 'struct %s' % field.vtype)
    if field.repeated:
      ctype = 'z_array_t'
    elif field.vtype == 'bytes':
      ctype = 'z_bytes_t *'
    return ctype

  def _get_def_value_type(self, vtype):
    ctype = self.C_PRIMITIVE_TYPES_MAP.get(vtype, 'struct %s' % vtype)
    if vtype == 'bytes':
      ctype = 'z_bytes_t *'
    return ctype

  def _get_value_type(self, vtype):
    return self.C_PRIMITIVE_TYPES_MAP.get(vtype, 'struct %s' % vtype)

  def _get_value_type_name(self, vtype):
    return self.C_PRIMITIVE_TYPES_NAME_MAP.get(vtype, vtype)

  def add_struct(self, entity):
    fields = []
    for field in entity.fields:
      ctype = self._get_def_type(field)
      fields.append('  %-10s %s;%s/* %2d: %s %s */' % (ctype,
          field.name, spacing(28, field.name), field.uid, field.type(), field.default or ''))

    has_macros = []
    for fid, field in enumerate(entity.fields):
      has_macros.append('#define {ENTITY_NAME}_has_%s(msg)  z_bitmap_test((msg)->{ENTITY_NAME}_fields_bitmap, %d)' % (field.name, fid))
      has_macros.append('#define {ENTITY_NAME}_set_%s(msg)  z_bitmap_set((msg)->{ENTITY_NAME}_fields_bitmap, %d)' % (field.name, fid))

    fields_free = []
    fields_alloc = []
    parse_fields = []
    for fid, field in enumerate(entity.fields):
      ctype = self._get_def_value_type(field.vtype)
      rvars = {
        '{FIELD_UID}': field.uid,
        '{FIELD_VTYPE}': self._get_value_type_name(field.vtype),
        '{FIELD_CTYPE}': ctype,
        '{FIELD_NAME}': field.name,
        '{FIELD_BITMAP_ID}': fid,
      }

      if field.repeated:
        if field.vtype == 'bytes':
          array_push = 'z_array_push_back_ptr(&(msg->{FIELD_NAME}), {FIELD_NAME})'
          fields_free.append(replace("""
  do {
    int i;
    for (i = 0; i < msg->{FIELD_NAME}.count; ++i) {
      z_bytes_t *value = (z_bytes_t *)z_array_get_ptr(&(msg->{FIELD_NAME}), z_bytes_t, i);
      z_bytes_free(value);
    }
  } while (0);
""", rvars))
        else:
          array_push = 'z_array_push_back_copy(&(msg->{FIELD_NAME}), &{FIELD_NAME})'
        rvars['{ARRAY_PUSH}'] = replace(array_push, rvars)

        fields_alloc.append('  z_array_open(&(msg->%s), sizeof(%s));' % (field.name, ctype))
        fields_free.append('  z_array_close(&(msg->%s));' % field.name)
        parse_blob = """
      case {FIELD_UID}: { /* list[{FIELD_VTYPE}] {FIELD_NAME} */
        {FIELD_CTYPE} {FIELD_NAME};
        if (Z_UNLIKELY(length > sizeof({FIELD_CTYPE}))) return(-1);
        r = z_reader_decode_{FIELD_VTYPE}(reader, length, &{FIELD_NAME});
        if (Z_UNLIKELY(r)) return(-1);
        r = {ARRAY_PUSH};
        if (Z_UNLIKELY(r)) return(-1);
        z_bitmap_set(msg->{ENTITY_NAME}_fields_bitmap, {FIELD_BITMAP_ID});
        break;
      }
"""
      elif field.vtype in 'bytes':
        fields_alloc.append('  msg->%s = NULL;' % field.name)
        fields_free.append('   z_bytes_free(msg->%s);' % field.name)
        parse_blob = """
      case {FIELD_UID}: { /* {FIELD_VTYPE} {FIELD_NAME} */
        if (Z_UNLIKELY(length > 1024)) return(-1);
        r = z_reader_decode_{FIELD_VTYPE}(reader, length, &(msg->{FIELD_NAME}));
        if (Z_UNLIKELY(r)) return(-1);
        z_bitmap_set(msg->{ENTITY_NAME}_fields_bitmap, {FIELD_BITMAP_ID});
        break;
      }
"""
      elif field.vtype in self.C_PRIMITIVE_TYPES_MAP:
        if field.default is not None:
          default_value = field.default.replace('false', '0').replace('true', '1')
          fields_alloc.append('  msg->%s = %s;' % (field.name, default_value))
          fields_alloc.append('  %s_set_%s(msg);' % (entity.name, field.name))

        parse_blob = """
      case {FIELD_UID}: { /* {FIELD_VTYPE} {FIELD_NAME} */
        if (Z_UNLIKELY(length > sizeof({FIELD_CTYPE}))) return(-1);
        r = z_reader_decode_{FIELD_VTYPE}(reader, length, &(msg->{FIELD_NAME}));
        if (Z_UNLIKELY(r)) return(-1);
        z_bitmap_set(msg->{ENTITY_NAME}_fields_bitmap, {FIELD_BITMAP_ID});
        break;
      }
"""
      else:
        fields_alloc.append(replace('  {FIELD_VTYPE}_alloc(&(msg->%s));' % field.name, rvars))
        fields_free.append(replace('   {FIELD_VTYPE}_free(&(msg->%s));' % field.name, rvars))
        parse_blob = """
      case {FIELD_UID}: { /* {FIELD_VTYPE} {FIELD_NAME} */
        r = {FIELD_VTYPE}_parse(&(msg->{FIELD_NAME}), reader, length);
        if (Z_UNLIKELY(r)) return(-1);
        z_bitmap_set(msg->{ENTITY_NAME}_fields_bitmap, {FIELD_BITMAP_ID});
        break;
      }
"""
      parse_fields.append(replace(parse_blob, rvars))

    dump_fields = []
    for field in entity.fields:
      if not field.vtype in self.C_PRIMITIVE_TYPES_MAP:
        continue

      ctype = self.C_PRIMITIVE_TYPES_MAP[field.vtype]

      if field.vtype == 'bytes':
        repeated_field_value = 'value'
      else:
        repeated_field_value = '*value'

      rvars = {
        '{FIELD_UID}': field.uid,
        '{FIELD_VTYPE}': self._get_value_type_name(field.vtype),
        '{FIELD_CTYPE}': ctype,
        '{FIELD_NAME}': field.name,
        '{REPEATED_FIELD_VALUE}': repeated_field_value,
      }

      parse_blob = """
  fprintf(stream, "\\n  {FIELD_UID}: {FIELD_VTYPE} {FIELD_NAME} = ");
  if ({ENTITY_NAME}_has_{FIELD_NAME}(msg)) {
"""
      if field.repeated:
        if field.vtype == 'bytes':
          array_get = 'z_array_get_ptr(&(msg->{FIELD_NAME}), {FIELD_CTYPE}, i)'
        else:
          array_get = 'z_array_get(&(msg->{FIELD_NAME}), {FIELD_CTYPE}, i)'
        rvars['{ARRAY_GET}'] = replace(array_get, rvars)

        parse_blob += """
    size_t i;
    for (i = 0; i < msg->{FIELD_NAME}.count; ++i) {
      const {FIELD_CTYPE} *value = {ARRAY_GET};
      fprintf(stream, "[%zu]", i);
      z_dump_{FIELD_VTYPE}(stream, {REPEATED_FIELD_VALUE});
    }
"""
      else:
        parse_blob += "    z_dump_{FIELD_VTYPE}(stream, msg->{FIELD_NAME});"

      parse_blob += """
  } else {
    fprintf(stream, "MISSING");
  }
"""
      dump_fields.append(replace(parse_blob, rvars))

    all_fields_write = []
    all_fields_size = []
    for field in entity.fields:
      ctype = self._get_value_type(field.vtype)
      value_field = 'msg->{FIELD_NAME}'

      if field.vtype == 'bytes':
        repeated_field_value = 'value'
      else:
        repeated_field_value = '*value'

      rvars = {
        '{FIELD_UID}': field.uid,
        '{FIELD_VTYPE}': self._get_value_type_name(field.vtype),
        '{FIELD_CTYPE}': ctype,
        '{FIELD_NAME}': field.name,
        '{FIELD_BITMAP_ID}': fid,
        '{VALUE_FIELD}': value_field,
        '{REPEATED_FIELD_VALUE}': repeated_field_value,
      }

      if field.repeated:
        if field.vtype == 'bytes':
          array_get = 'z_array_get_ptr(&(msg->{FIELD_NAME}), {FIELD_CTYPE}, i)'
        else:
          array_get = 'z_array_get(&(msg->{FIELD_NAME}), {FIELD_CTYPE}, i)'
        rvars['{ARRAY_GET}'] = replace(array_get, rvars)

        parse_blob_write = """
  if ({ENTITY_NAME}_has_{FIELD_NAME}(msg)) {
    size_t i;
    for (i = 0; i < msg->{FIELD_NAME}.count; ++i) {
      const {FIELD_CTYPE} *value = {ARRAY_GET};
      /* TODO WRITE struct */
      r = z_write_field_{FIELD_VTYPE}(buffer, {FIELD_UID}, {REPEATED_FIELD_VALUE});
      if (Z_UNLIKELY(r)) return(-{FIELD_UID});
    }
  }
"""
        parse_blob_size = "/* TODO {FIELD_NAME} */"
      elif field.vtype in self.C_PRIMITIVE_TYPES_MAP:
        parse_blob_write = """
  if ({ENTITY_NAME}_has_{FIELD_NAME}(msg)) {
    r = z_write_field_{FIELD_VTYPE}(buffer, {FIELD_UID}, {VALUE_FIELD});
    if (Z_UNLIKELY(r)) return(-{FIELD_UID});
  }
"""
        if field.vtype == 'bytes':
          parse_blob_size = """
  if ({ENTITY_NAME}_has_{FIELD_NAME}(msg)) {
    size += z_encoded_field_length({FIELD_UID}, {VALUE_FIELD}->slice.length);
    size += {VALUE_FIELD}->slice.length;
  }
"""
        else:
          parse_blob_size = """
  if ({ENTITY_NAME}_has_{FIELD_NAME}(msg)) {
    unsigned int length = z_{FIELD_VTYPE}_size({VALUE_FIELD});
    size += z_encoded_field_length({FIELD_UID}, length) + length;
  }
"""
      else:
        parse_blob_write = """
  if ({ENTITY_NAME}_has_{FIELD_NAME}(msg)) {
    unsigned int size = {FIELD_VTYPE}_size(&(msg->{FIELD_NAME}));
    z_write_field(buffer, {FIELD_UID}, size);
    r = {FIELD_VTYPE}_write(&(msg->{FIELD_NAME}), buffer);
    if (Z_UNLIKELY(r)) return(-{FIELD_UID});
  }
"""
        parse_blob_size = """
  if ({ENTITY_NAME}_has_{FIELD_NAME}(msg)) {
    size += {FIELD_VTYPE}_size(&(msg->{FIELD_NAME}));
  }
"""

      all_fields_write.append(replace(parse_blob_write, rvars))
      all_fields_size.append(replace(parse_blob_size, rvars))

    if entity.has_rpc_head:
      rpc_head = "uint64_t rpc_head[2];       /* Request (Id, Type) */"
    else:
      rpc_head = ''

    rheaders = {
      '{RPC_HEAD}': rpc_head,
      '{ENTITY_FIELDS}': '\n'.join(fields),
      '{ENTITY_HAS_MACROS}': '\n'.join(has_macros),
    }

    rsources = {
      '{FIELDS_PARSE}': '\n'.join(parse_fields),
      '{FIELDS_ALLOC}': '\n'.join(fields_alloc),
      '{FIELDS_FREE}': '\n'.join(fields_free),
      '{ALL_FIELDS_SIZE}':  '\n'.join(all_fields_size),
      '{ALL_FIELDS_WRITE}':  '\n'.join(all_fields_write),
      '{FIELDS_DUMP}': '\n'.join(dump_fields),
    }

    rvars = {
      '{ENTITY_NAME}': entity.name,
      '{ENTITY_BITMAP_SIZE}': ceil(len(entity.fields), 8),
    }

    self.headers.append(replace("""
struct {ENTITY_NAME} {
  {RPC_HEAD}

  /* Internal states */
  uint8_t {ENTITY_NAME}_fields_bitmap[{ENTITY_BITMAP_SIZE}];
  int {ENTITY_NAME}_ialloc;

  /* Fields */
{ENTITY_FIELDS}
};

{ENTITY_HAS_MACROS}

struct {ENTITY_NAME} *{ENTITY_NAME}_alloc (struct {ENTITY_NAME} *msg);
void   {ENTITY_NAME}_free (struct {ENTITY_NAME} *msg);
int    {ENTITY_NAME}_parse (struct {ENTITY_NAME} *msg, void *reader, uint64_t size);
size_t {ENTITY_NAME}_size  (struct {ENTITY_NAME} *msg);
int    {ENTITY_NAME}_write (struct {ENTITY_NAME} *msg, z_buffer_t *buffer);
void   {ENTITY_NAME}_dump  (FILE *stream, const struct {ENTITY_NAME} *msg);
""", rheaders, rvars))

    self.sources.append(replace("""
struct {ENTITY_NAME} *{ENTITY_NAME}_alloc (struct {ENTITY_NAME} *msg) {
  if (msg == NULL) {
    msg = z_memory_struct_alloc(z_global_memory(), struct {ENTITY_NAME});
    if (Z_MALLOC_IS_NULL(msg))
      return(NULL);
    msg->{ENTITY_NAME}_ialloc = 1;
  } else {
    msg->{ENTITY_NAME}_ialloc = 0;
  }

  z_memzero(msg->{ENTITY_NAME}_fields_bitmap, {ENTITY_BITMAP_SIZE});
{FIELDS_ALLOC}
  return(msg);
}

void {ENTITY_NAME}_free (struct {ENTITY_NAME} *msg) {
{FIELDS_FREE}
  if (msg->{ENTITY_NAME}_ialloc)
    z_memory_struct_free(z_global_memory(), struct {ENTITY_NAME}, msg);
}

int {ENTITY_NAME}_parse (struct {ENTITY_NAME} *msg, void *reader, uint64_t size) {
  uint64_t total_length = 0;
  uint16_t field_id;
  uint64_t length;
  int r;

  while (z_reader_decode_field(reader, &field_id, &length) > 0) {
    switch (field_id) {
      {FIELDS_PARSE}

      default: {
        r = -1;
        if (z_reader_skip(reader, length))
          return(-1);
        break;
      }
    }

    total_length += length;
    if (total_length >= size)
      break;
  }

  return(0);
}

size_t {ENTITY_NAME}_size (struct {ENTITY_NAME} *msg) {
  size_t size = 0;
  {ALL_FIELDS_SIZE}
  return(size);
}

int {ENTITY_NAME}_write (struct {ENTITY_NAME} *msg, z_buffer_t *buffer) {
  int r = 0;
  {ALL_FIELDS_WRITE}
  return(r);
}

void {ENTITY_NAME}_dump (FILE *stream, const struct {ENTITY_NAME} *msg) {
  fprintf(stream, "{ENTITY_NAME} {");
  {FIELDS_DUMP}
  fprintf(stream, "\\n}\\n");
}
""", rsources, rvars))

  def add_rpc(self, entity):
    rpc_methods = []
    for call in entity.calls:
      spaces = ' ' * len(call.name)
      rpc_methods.append("  int (*%s) (z_ipc_client_t *client,\n" % (call.name) +
                         "        %s   struct %s *req,\n" % (spaces, call.atype) +
                         "        %s   struct %s *resp);" % (spaces, call.rtype))

    req_handle = []
    resp_handle = []
    for call in entity.calls:
      rvars = {
        '{REQ_ID}': call.uid,
        '{REQ_NAME}': call.name,
        '{REQ_ATYPE}': call.atype,
        '{REQ_RTYPE}': call.rtype,
        '{REQ_TYPE_ID}': call.rtype.upper(),
        '{RESP_TYPE_ID}': call.rtype.upper(),
      }

      req_handle.append(replace("""
    case {REQ_ID}: { /* {REQ_ID} */
      struct {REQ_RTYPE} *resp;
      struct {REQ_ATYPE} *req;

      Z_LOG_TRACE("Handling RPC Request %lu Type %lu {REQ_NAME} from client %d",
                  req_id, msg_type, Z_IOPOLL_ENTITY_FD(client));

      req = {REQ_ATYPE}_alloc(NULL);
      if (Z_MALLOC_IS_NULL(req)) {
        Z_LOG_FATAL("Unable to allocate request {REQ_ATYPE}");
        break;
      }

      if ({REQ_ATYPE}_parse(req, &reader, size)) {
        Z_LOG_FATAL("Unable to parse request {REQ_ATYPE}");
        {REQ_ATYPE}_free(req);
        break;
      }

      req->rpc_head[0] = {REQ_ID};
      req->rpc_head[1] = req_id;

      resp = {REQ_RTYPE}_alloc(NULL);
      if (Z_MALLOC_IS_NULL(resp)) {
        Z_LOG_FATAL("Unable to allocate response {REQ_RTYPE}");
        {REQ_ATYPE}_free(req);
        break;
      }

      if ((r = proto->{REQ_NAME}(client, req, resp))) {
        {REQ_RTYPE}_free(resp);
        {REQ_ATYPE}_free(req);
      }
      break;
    }
""", rvars))

      resp_handle.append(replace("""
    case {REQ_ID}: { /* {REQ_ID} */
      {REQ_ATYPE}_free(req);
      r = {REQ_RTYPE}_write((struct {REQ_RTYPE} *)resp, buffer);
      {REQ_RTYPE}_free(resp);
      break;
    }
""", rvars))

    rheaders = {
      '{RPC_METHODS}': '\n'.join(rpc_methods),
    }

    rsources = {
      '{REQ_HANDLE}': '\n'.join(req_handle),
      '{RESP_HANDLE}':  '\n'.join(resp_handle),
    }

    rvars = {
      '{ENTITY_NAME}': entity.name,
    }

    self.headers_server.append(replace("""
struct {ENTITY_NAME}_server {
{RPC_METHODS}
};

int  {ENTITY_NAME}_server_parse (const struct {ENTITY_NAME}_server *proto,
                                 z_ipc_client_t *client,
                                 const struct iovec iov[2]);
int  {ENTITY_NAME}_server_push_response (z_ipc_client_t *client,
                                         z_ipc_msgbuf_t *msgbuf,
                                         void *req, void *resp);
""", rheaders, rvars))

    self.sources_server.append(replace("""
int {ENTITY_NAME}_server_parse (const struct {ENTITY_NAME}_server *proto,
                                z_ipc_client_t *client,
                                const struct iovec iov[2])
{
  z_iovec_reader_t reader;
  z_memory_t *memory;
  uint64_t msg_type;
  uint64_t req_id;
  uint64_t size;
  int r = -1;

  z_iovec_reader_open(&reader, iov, 2);
  size = z_reader_available(&reader);
  memory = z_global_memory();

  /* Parse the RPC header */
  if (z_reader_decode_uint64(&reader, 8, &msg_type)) {
    Z_LOG_FATAL("Unable to read the Message-Type from the RPC header");
    return(-1);
  }

  if (z_reader_decode_uint64(&reader, 8, &req_id)) {
    Z_LOG_FATAL("Unable to read the Request-ID from the RPC header");
    return(-1);
  }

  switch (msg_type) {
{REQ_HANDLE}

    default:
      Z_LOG_FATAL("Unknown type %u for request %u", msg_type, req_id);
      break;
  }

  z_iovec_reader_close(&reader);
  return(r);
}

int  {ENTITY_NAME}_server_push_response (z_ipc_client_t *client,
                                         z_ipc_msgbuf_t *msgbuf,
                                         void *req, void *resp)
{
  z_memory_t *memory;
  uint64_t *rpc_head;
  z_buffer_t *buffer;
  int r = -1;

  memory = z_global_memory();
  if ((buffer = z_buffer_alloc(NULL)) == NULL) {
    Z_LOG_FATAL("TODO unable to allocate buffer (free req/resp)\\n");
    return(-1);
  }

  if (z_buffer_reserve(buffer, 128)) {
    Z_LOG_FATAL("TODO unable to reserve buffer space (free req/resp)\\n");
    return(-1);
  }

  /* Write the RPC header */
  rpc_head = Z_UINT64_PTR(req);
  z_encode_uint(buffer->block + 0, 8, *rpc_head++);
  z_encode_uint(buffer->block + 8, 8, *rpc_head++);
  buffer->size = 16;

  /* Write the Response */
  switch (Z_UINT64_PTR_VALUE(req)) {
{RESP_HANDLE}

    default:
      Z_LOG_FATAL("Unknown type %u for request\\n", Z_UINT64_PTR_VALUE(req));
      break;
  }

  Z_LOG_TRACE("Send response of size=%zu", buffer->size);
  z_ipc_msgbuf_push(msgbuf, buffer->block, buffer->size);
  z_ipc_client_set_writable(client, 1);
  z_buffer_release(buffer);
  z_buffer_free(buffer);

  return(r);
}
""", rsources, rvars))

  def write(self, outdir=None):
    if self.headers:
      fd_h, fd_s = self._open_wfiles(outdir)
      self.write_header(fd_h)
      self.write_source(fd_s)

    if self.headers_client:
      fd_h, fd_s = self._open_wfiles(outdir, 'client')
      self.write_client_header(fd_h)
      self.write_client_source(fd_s)

    if self.headers_server:
      fd_h, fd_s = self._open_wfiles(outdir, 'server')
      self.write_server_header(fd_h)
      self.write_server_source(fd_s)

  def _open_wfiles(self, outdir, suffix=''):
    if outdir:
      path = os.path.join(outdir, self.module)
      if suffix: path += '_' + suffix
      fd_h = open(path + '.h', 'w')
      fd_s = open(path + '.c', 'w')
      print path + '.{h,c}'
    else:
      fd_h = sys.stdout
      fd_s = sys.stdout
    return fd_h, fd_s

  def write_header(self, fd):
    header_vars = {
      '{MODULE_NAME_UPPER}': self.module.upper(),
      '{HEADER_DATA}': '\n'.join(self.headers)
    }

    self.write_blob(fd, header_vars, """
/* File autogenerated, do not edit */
#ifndef _{MODULE_NAME_UPPER}_H_
#define _{MODULE_NAME_UPPER}_H_

#include <zcl/ipc.h>
#include <zcl/macros.h>
#include <zcl/coding.h>
#include <zcl/reader.h>
#include <zcl/writer.h>
#include <zcl/array.h>
#include <zcl/buffer.h>
#include <zcl/bitmap.h>
#include <stdio.h>

{HEADER_DATA}

#endif /* !_{MODULE_NAME_UPPER}_H_ */
""")

  def write_client_header(self, fd):
    header_vars = {
      '{MODULE_NAME}': self.module,
      '{MODULE_NAME_UPPER}': self.module.upper(),
      '{HEADER_DATA}': '\n'.join(self.headers_client)
    }

    self.write_blob(fd, header_vars, """
/* File autogenerated, do not edit */
#ifndef _{MODULE_NAME_UPPER}_CLIENT_H_
#define _{MODULE_NAME_UPPER}_CLIENT_H_

#include <zcl/ipc.h>
#include "{MODULE_NAME}.h"

{HEADER_DATA}

#endif /* !_{MODULE_NAME_UPPER}_CLIENT_H_ */
""")

  def write_server_header(self, fd):
    header_vars = {
      '{MODULE_NAME}': self.module,
      '{MODULE_NAME_UPPER}': self.module.upper(),
      '{HEADER_DATA}': '\n'.join(self.headers_server)
    }

    self.write_blob(fd, header_vars, """
/* File autogenerated, do not edit */
#ifndef _{MODULE_NAME_UPPER}_SERVER_H_
#define _{MODULE_NAME_UPPER}_SERVER_H_

#include <zcl/ipc.h>
#include "{MODULE_NAME}.h"

{HEADER_DATA}

#endif /* !_{MODULE_NAME_UPPER}_SERVER_H_ */
""")

  def write_source(self, fd):
    source_vars = {
      '{MODULE_NAME}': (self.module),
      '{SOURCE_DATA}': '\n'.join(self.sources)
    }

    self.write_blob(fd, source_vars, """
/* File autogenerated, do not edit */
#include <zcl/global.h>
#include <zcl/string.h>
#include <zcl/debug.h>
#include "{MODULE_NAME}.h"

{SOURCE_DATA}
""")

  def write_client_source(self, fd):
    source_vars = {
      '{MODULE_NAME}': (self.module),
      '{SOURCE_DATA}': '\n'.join(self.sources_client)
    }

    self.write_blob(fd, source_vars, """
/*
 * File autogenerated, do not edit.
 * (this file should be part of the client library)
 */
#include <zcl/string.h>
#include "{MODULE_NAME}_client.h"

{SOURCE_DATA}
""")

  def write_server_source(self, fd):
    source_vars = {
      '{MODULE_NAME}': (self.module),
      '{SOURCE_DATA}': '\n'.join(self.sources_server)
    }

    self.write_blob(fd, source_vars, """
/* File autogenerated, do not edit */
#include <zcl/string.h>
#include <zcl/global.h>
#include <zcl/iovec.h>
#include <zcl/debug.h>

#include "{MODULE_NAME}_server.h"

{SOURCE_DATA}
""")

# =============================================================================
#  Entity parsing
# =============================================================================
RX_ENTITY = re.compile('\s*(\w+)\s+(\w+)\s*\\{([^}]*)\\}', re.MULTILINE)
RX_ENTITY_FIELD = re.compile('\s*([0-9]+)\s*:\s*([^;]+){1,}', re.MULTILINE)

def extract_entities(data):
  data = re.sub(r'/\*[^*/]*\*/', '', data)
  for entity in RX_ENTITY.finditer(data):
    e_type, e_name, e_body = entity.groups()
    e_fields = []
    for field in RX_ENTITY_FIELD.finditer(e_body):
      field_id, field_body = field.groups()
      e_fields.append((int(field_id), field_body))
    yield e_type, e_name, e_fields

def parse_entities(data):
  for e_type, e_name, e_fields in extract_entities(data):
    try:
      func = globals()['parse_%s_entity' % e_type]
      yield func(e_name, e_fields)
    except KeyError:
      raise Exception("Unknown entity type '%s' used by '%s'" % (e_type, e_name))

def parse_entities_from_file(path):
  fd = open(path)
  try:
    return parse_entities(fd.read())
  finally:
    fd.close()

def parse_struct_entity(name, fields):
  RX_STRUCT_FIELD = re.compile('([0-9_a-z\\[\\]]+)\s+(\w+)\s*(\\[.*\\])*')
  entity = StructEntity(name)
  for field_id, field_body in fields:
    field_type, field_name, defaults = RX_STRUCT_FIELD.match(field_body).groups()
    if defaults is not None: defaults = defaults[1:-1].split('=')[1].strip()
    entity.add_field(field_id, field_type, field_name, defaults)
  return entity

def parse_request_entity(name, fields):
  request = parse_struct_entity(name + '_request', fields)
  request.has_rpc_head = True
  return request

def parse_response_entity(name, fields):
  return parse_struct_entity(name + '_response', fields)

def parse_rpc_entity(name, fields):
  RX_STRUCT_FIELD = re.compile('(\w+)')
  entity = RpcEntity(name)
  for call_id, call_name in fields:
    entity.add_call(call_id, call_name, call_name + '_response', call_name + '_request')
  return entity

class StructEntity(object):
  etype = 'struct'

  class Field(object):
    def __init__(self, uid, vtype, name, default):
      if uid < 0 or uid >= 64:
        raise Exception("Invalid field uid: %d" % uid)
      if vtype.startswith('list'):
        vtype = re.match('list\[(\w+)\]', vtype).groups()[0]
        self.repeated = True
      else:
        self.repeated = False
      # TODO: Validate vtype
      self.default = default
      self.vtype = vtype
      self.name = name
      self.uid = uid

    def type(self):
      if self.repeated:
        return 'list[%s]' % self.vtype
      return self.vtype

    def __repr__(self):
      s = '%2d: %s %s' % (self.uid, self.type(), self.name)
      if self.default is not None:
        s += ' [%s]' % self.default
      return s + ';'

  def __init__(self, name):
    self.name = name
    self.fields = []
    self.has_rpc_head = False

  def add_field(self, uid, vtype, name, default):
    if len(self.fields) >= 128:
      raise Exception("Max number of field reached")
    self.fields.append(self.Field(uid, vtype, name, default))

  def __repr__(self):
    return 'struct %s {\n  %s\n};' % (self.name, '\n  '.join(map(str, self.fields)))

class RpcEntity(object):
  etype = 'rpc'

  class Call(object):
    def __init__(self, uid, name, rtype, atype):
      self.uid = uid
      self.name = name
      self.rtype = rtype
      self.atype = atype

    def __repr__(self):
      return '%2d: %s %s (%s);' % (self.uid, self.rtype, self.name, self.atype)

  def __init__(self, name):
    self.name = name
    self.calls = []

  def add_call(self, uid, name, rtype, atype):
    self.calls.append(self.Call(uid, name, rtype, atype))

  def __repr__(self):
    return 'rpc %s {\n  %s\n};' % (self.name, '\n  '.join(map(str, self.calls)))

DEMO_STRUCT = """
/*
struct foo {
  0: uint8  a;
  1: uint16 b;
  2: uint64 a;
  3: uint32 c;
  4: list[uint64] d;
};

struct bar {
  1: foo x;
};
*/
rpc zrpc {
  0: foo test (bar);

  /* Semantic */
  10: foo semantic_create (bar);
  11: foo semantic_delete (bar);
  12: foo semantic_rename (bar);
  13: foo semantic_exists (bar);

  /* Counter */
  20: foo counter_get (bar);
  21: foo counter_set (bar);
  22: foo counter_cas (bar);
  23: foo counter_inc (bar);
  24: foo counter_dec (bar);

  /* Key/Value */
  /* Sorted Set */
  /* Deque */
  /* Struct */

  /* Server */
  30: foo server_ping (bar);
  31: foo server_info (bar);
  32: foo server_quit (bar);
}
"""

def main(argv):
  if len(argv) < 3:
    print 'usage: zcl-compiler [output dir] [message file]'
    sys.exit(1)

  out_dir = argv[1]
  if not os.path.exists(out_dir): os.makedirs(out_dir)

  for filename in argv[2:]:
    name, _ = os.path.splitext(os.path.basename(filename))
    cgen = CCodeGenerator(name)
    for entity in parse_entities_from_file(filename):
      cgen.add(entity)
    cgen.write(out_dir)

if __name__ == '__main__':
  if 1:
    main(sys.argv)
  else:
    cgen = CCodeGenerator('test')
    for entity in parse_entities(DEMO_STRUCT):
      cgen.add(entity)
    cgen.write()
  print