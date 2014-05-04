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
    self.add_rpc_server(entity)
    self.add_rpc_client(entity)

  def add_rpc_client(self, entity):
    raise NotImplementedError

  def add_rpc_server(self, entity):
    raise NotImplementedError

  def write(self, outdir):
    raise NotImplementedError

  def write_blob(self, fd, rvars, blob):
    fd.write(replace(blob, rvars))

PRIMITIVE_TYPES_MAX_SIZE = {
  'bool':  1,
  'int8':  1, 'int16':  2, 'int32':  4, 'int64': 8,
  'uint8': 1, 'uint16': 2, 'uint32': 4, 'uint64': 8,
  'bytes': 2 ** 32,
}

# =============================================================================
#  C code generator
# =============================================================================
class CCodeGenerator(CodeGenerator):
  C_PRIMITIVE_TYPES_MAP = {
    'bool': 'int8_t',
    'int8': 'int8_t', 'int16': 'int16_t', 'int32': 'int32_t', 'int64': 'int64_t',
    'uint8': 'uint8_t', 'uint16': 'uint16_t', 'uint32': 'uint32_t', 'uint64': 'uint64_t',
    'bytes': 'z_memref_t', 'string': 'z_memref_t',
  }

  C_PRIMITIVE_TYPES_NAME_MAP = {'bool': 'int8'}
  C_PRIMITIVE_TYPES_BLOB = ('bytes', 'string')

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
    return ctype

  def _get_def_value_type(self, vtype):
    ctype = self.C_PRIMITIVE_TYPES_MAP.get(vtype, 'struct %s' % vtype)
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
        array_push = 'z_array_push_back_copy(&(msg->{FIELD_NAME}), &{FIELD_NAME})'
        rvars['{ARRAY_PUSH}'] = replace(array_push, rvars)

        fields_alloc.append('  z_array_open(&(msg->%s), sizeof(%s));' % (field.name, ctype))

        if field.vtype in self.C_PRIMITIVE_TYPES_BLOB:
          parse_blob = """
  do {
    size_t i;
    for (i = 0; i < msg->{FIELD_NAME}.count; ++i) {
      {FIELD_CTYPE} *value = z_array_get(&(msg->{FIELD_NAME}), {FIELD_CTYPE}, i);
      z_memref_release(value);
    }
  } while (0);
"""
          fields_free.append(replace(parse_blob, rvars))

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
      elif field.vtype in self.C_PRIMITIVE_TYPES_BLOB:
        fields_alloc.append('  z_memref_reset(&(msg->%s));' % field.name)
        fields_free.append('   z_memref_release(&(msg->%s));' % field.name)
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
      if field.vtype in self.C_PRIMITIVE_TYPES_BLOB:
        value_field = '&(msg->{FIELD_NAME})'
        repeated_field_value = 'value'
      else:
        value_field = 'msg->{FIELD_NAME}'
        repeated_field_value = '*value'

      rvars = {
        '{FIELD_UID}': field.uid,
        '{FIELD_VTYPE}': self._get_value_type_name(field.vtype),
        '{FIELD_CTYPE}': ctype,
        '{FIELD_NAME}': field.name,
        '{VALUE_FIELD}': value_field,
        '{REPEATED_FIELD_VALUE}': repeated_field_value,
      }

      parse_blob = """
  fprintf(stream, "\\n  {FIELD_UID}: {FIELD_VTYPE} {FIELD_NAME} = ");
  if ({ENTITY_NAME}_has_{FIELD_NAME}(msg)) {
"""
      if field.repeated:
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
        parse_blob += "    z_dump_{FIELD_VTYPE}(stream, {VALUE_FIELD});"

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

      if field.vtype in self.C_PRIMITIVE_TYPES_BLOB:
        value_field = '&(msg->{FIELD_NAME})'
        repeated_field_value = 'value'
      else:
        value_field = 'msg->{FIELD_NAME}'
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
        array_get = 'z_array_get(&(msg->{FIELD_NAME}), {FIELD_CTYPE}, i)'
        rvars['{ARRAY_GET}'] = replace(array_get, rvars)

        # TODO WRITE struct
        parse_blob_write = """
  if ({ENTITY_NAME}_has_{FIELD_NAME}(msg)) {
    size_t i;
    for (i = 0; i < msg->{FIELD_NAME}.count; ++i) {
      const {FIELD_CTYPE} *value = {ARRAY_GET};
      r = z_write_field_{FIELD_VTYPE}(writer, {FIELD_UID}, {REPEATED_FIELD_VALUE});
      if (Z_UNLIKELY(r)) return(-{FIELD_UID});
    }
  }
"""
        parse_blob_size = """
        /* TODO {FIELD_NAME} */
  if ({ENTITY_NAME}_has_{FIELD_NAME}(msg)) {
    size_t i;
    for (i = 0; i < msg->{FIELD_NAME}.count; ++i) {
      const {FIELD_CTYPE} *value = {ARRAY_GET};
      size += z_field_encoded_length({FIELD_UID}, msg->{FIELD_NAME}.slice.size);
    }
  }
"""

        if field.vtype in self.C_PRIMITIVE_TYPES_BLOB:
          parse_blob_size = """
  if ({ENTITY_NAME}_has_{FIELD_NAME}(msg)) {
    size_t i;
    for (i = 0; i < msg->{FIELD_NAME}.count; ++i) {
      const {FIELD_CTYPE} *value = {ARRAY_GET};
      size += z_field_encoded_length({FIELD_UID}, value->slice.size);
      size += value->slice.size;
    }
  }
"""
        else:
          parse_blob_size = """
  if ({ENTITY_NAME}_has_{FIELD_NAME}(msg)) {
    size_t i;
    for (i = 0; i < msg->{FIELD_NAME}.count; ++i) {
      const {FIELD_CTYPE} *value = {ARRAY_GET};
      size_t length = z_{FIELD_VTYPE}_size(*value);
      size += z_field_encoded_length({FIELD_UID}, length) + length;
    }
  }
"""

      elif field.vtype in self.C_PRIMITIVE_TYPES_MAP:
        parse_blob_write = """
  if ({ENTITY_NAME}_has_{FIELD_NAME}(msg)) {
    r = z_write_field_{FIELD_VTYPE}(writer, {FIELD_UID}, {VALUE_FIELD});
    if (Z_UNLIKELY(r)) return(-{FIELD_UID});
  }
"""
        if field.vtype in self.C_PRIMITIVE_TYPES_BLOB:
          parse_blob_size = """
  if ({ENTITY_NAME}_has_{FIELD_NAME}(msg)) {
    size += z_field_encoded_length({FIELD_UID}, msg->{FIELD_NAME}.slice.size);
    size += msg->{FIELD_NAME}.slice.size;
  }
"""
        else:
          parse_blob_size = """
  if ({ENTITY_NAME}_has_{FIELD_NAME}(msg)) {
    size_t length = z_{FIELD_VTYPE}_size({VALUE_FIELD});
    size += z_field_encoded_length({FIELD_UID}, length) + length;
  }
"""
      else:
        parse_blob_write = """
  if ({ENTITY_NAME}_has_{FIELD_NAME}(msg)) {
    size_t size = {FIELD_VTYPE}_size(&(msg->{FIELD_NAME}));
    z_write_field(writer, {FIELD_UID}, size);
    r = {FIELD_VTYPE}_write(&(msg->{FIELD_NAME}), writer);
    if (Z_UNLIKELY(r)) return(-{FIELD_UID});
  }
"""
        parse_blob_size = """
  if ({ENTITY_NAME}_has_{FIELD_NAME}(msg)) {
    size_t length = {FIELD_VTYPE}_size(&(msg->{FIELD_NAME}));
    size += z_field_encoded_length({FIELD_UID}, length) + length;
  }
"""

      all_fields_write.append(replace(parse_blob_write, rvars))
      all_fields_size.append(replace(parse_blob_size, rvars))

    rheaders = {
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
      '{ENTITY_BITMAP_SIZE}': max(1, ceil(len(entity.fields), 8)),
    }

    self.headers.append(replace("""

typedef struct {ENTITY_NAME} {
  /* Internal states */
  uint8_t {ENTITY_NAME}_fields_bitmap[{ENTITY_BITMAP_SIZE}];
  uint8_t {ENTITY_NAME}_ialloc;

  /* Fields */
{ENTITY_FIELDS}
} {ENTITY_NAME}_t;

{ENTITY_HAS_MACROS}

struct {ENTITY_NAME} *{ENTITY_NAME}_alloc (struct {ENTITY_NAME} *msg);
void   {ENTITY_NAME}_free (struct {ENTITY_NAME} *msg);
int    {ENTITY_NAME}_parse (struct {ENTITY_NAME} *msg, void *reader, uint64_t size);
size_t {ENTITY_NAME}_size  (struct {ENTITY_NAME} *msg);
int    {ENTITY_NAME}_write (struct {ENTITY_NAME} *msg, z_dbuf_writer_t *writer);
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
  int r = 0;

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
    if (Z_UNLIKELY(total_length >= size))
      break;
  }

  return(r);
}

size_t {ENTITY_NAME}_size (struct {ENTITY_NAME} *msg) {
  size_t size = 0;
  {ALL_FIELDS_SIZE}
  return(size);
}

int {ENTITY_NAME}_write (struct {ENTITY_NAME} *msg, z_dbuf_writer_t *writer) {
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

  def add_rpc_client(self, entity):
    rpc_ids = []
    for call in entity.calls:
      rpc_ids.append('#define %s_ID %d' % (call.name.upper(), call.uid))

    req_handle = []
    req_builder = []
    resp_handle = []
    for call in entity.calls:
      rvars = {
        '{MSG_ID}': '%s_ID' % call.name.upper(),
        '{MSG_NAME}': call.name,
        '{MSG_REQ_TYPE}': call.atype,
        '{MSG_RESP_TYPE}': call.rtype,
        '{REQ_TYPE_ID}': call.rtype.upper(),
        '{RESP_TYPE_ID}': call.rtype.upper(),
      }

      resp_handle.append(replace("""
    case {MSG_ID}: {
      struct {MSG_RESP_TYPE} *resp = NULL;

      resp = {MSG_RESP_TYPE}_alloc(resp);
      if (Z_MALLOC_IS_NULL(resp)) {
        Z_LOG_FATAL("Unable to allocate response {MSG_RESP_TYPE}");
        /* TODO */
        break;
      }

      if ((r = {MSG_RESP_TYPE}_parse(resp, reader, size))) {
        Z_LOG_FATAL("Unable to parse message {REQ_ATYPE}");
        {MSG_RESP_TYPE}_free(resp);
        /* TODO */
        break;
      }

      call->callback(call->ctx, call->ucallback, call->udata);
      {MSG_RESP_TYPE}_free(resp);
      /* TODO */
      break;
    }
""", rvars))

      req_handle.append(replace("""
    case {MSG_ID}: {
      r = {MSG_REQ_TYPE}_write(z_rpc_ctx_request(ctx, {MSG_NAME}), &writer);
      break;
    }
""", rvars))

      req_builder.append(replace("""
    case {MSG_ID}: {
      ctx = z_rpc_ctx_alloc(struct {MSG_REQ_TYPE}, struct {MSG_RESP_TYPE});
      if (Z_MALLOC_IS_NULL(ctx)) {
        Z_LOG_FATAL("Unable to allocate rpc context {MSG_NAME}");
        return(NULL);
      }

      /* TODO: CHECK ME! */
      {MSG_REQ_TYPE}_alloc(z_rpc_ctx_request(ctx, {MSG_NAME}));
      {MSG_RESP_TYPE}_alloc(z_rpc_ctx_response(ctx, {MSG_NAME}));
      break;
    }
""", rvars))

    rvars = {
      '{ENTITY_NAME}': entity.name,
      '{RPC_IDS}': '\n'.join(rpc_ids),
    }

    rheaders = {}
    rsources = {'{REQ_HANDLE}': '\n'.join(req_handle),
                '{RESP_HANDLE}': '\n'.join(resp_handle),
                '{REQ_BUILDER}': '\n'.join(req_builder),
               }

    self.headers_client.append(replace("""
{RPC_IDS}

z_rpc_ctx_t *{ENTITY_NAME}_client_build_request (z_ipc_msg_client_t *client,
                                                 uint64_t msg_type,
                                                 uint64_t req_id);

int  {ENTITY_NAME}_client_parse (z_rpc_map_t *rpc_map,
                                 const struct iovec iov[2]);
int  {ENTITY_NAME}_client_push_request (z_rpc_ctx_t *ctx,
                                        z_rpc_map_t *rpc_map,
                                        void *sys_callback,
                                        void *ucallback,
                                        void *udata);
""", rheaders, rvars))

    self.sources_client.append(replace("""
static int __{ENTITY_NAME}_client_parse_request (z_iovec_reader_t *reader,
                                                 uint64_t msg_type,
                                                 uint64_t req_id)
{
  return(-1);
}

static int __{ENTITY_NAME}_client_parse_response (z_iovec_reader_t *reader,
                                                  z_rpc_map_t *rpc_map,
                                                  uint64_t msg_type,
                                                  uint64_t req_id)
{
  z_rpc_call_t *call;
  uint64_t size;
  int r = -1;

  size = z_reader_available(reader);
  call = z_rpc_map_remove(rpc_map, req_id);

  switch (msg_type) {
{RESP_HANDLE}

    default:
      /* ctx->handle_unknown(client, Z_IPC_UNKNOWN_MESSAGE, msg_type, req_id); */
      Z_LOG_FATAL("Unknown type %u for response %u", msg_type, req_id);
      break;
  }

  return(r);
}

int {ENTITY_NAME}_client_parse (z_rpc_map_t *rpc_map,
                                const struct iovec iov[2])
{
  z_iovec_reader_t reader;
  uint64_t msg_type;
  uint64_t req_id;
  int is_req;
  int r = -1;

  z_iovec_reader_open(&reader, iov, 2);

  /* Parse the RPC header */
  if (z_rpc_parse_head(&reader, &msg_type, &req_id, &is_req)) {
    Z_LOG_FATAL("Unable to read the Request-Head from the RPC header");
    return(-1);
  }

  if (Z_UNLIKELY(is_req)) {
    r = __{ENTITY_NAME}_client_parse_request(&reader, msg_type, req_id);
  } else {
    r = __{ENTITY_NAME}_client_parse_response(&reader, rpc_map, msg_type, req_id);
  }

  z_iovec_reader_close(&reader);
  return(r);
}

z_rpc_ctx_t *{ENTITY_NAME}_client_build_request (z_ipc_msg_client_t *client,
                                                 uint64_t msg_type,
                                                 uint64_t req_id)
{
  z_rpc_ctx_t *ctx;

  switch (msg_type) {
{REQ_BUILDER}

    default:
      Z_LOG_FATAL("Unknown type %"PRIu64" for request\\n", msg_type);
      return(NULL);
  }

  ctx->client   = client;
  ctx->req_id   = req_id;
  ctx->req_time = z_time_micros();
  ctx->msg_type = msg_type;
  return(ctx);
}

int {ENTITY_NAME}_client_push_request (z_rpc_ctx_t *ctx,
                                       z_rpc_map_t *rpc_map,
                                       void *sys_callback,
                                       void *ucallback,
                                       void *udata)
{
  z_rpc_call_t *call;
  z_dbuf_writer_t writer;
  uint8_t head[16];
  int r = -1;

  call = z_rpc_map_add(rpc_map, ctx, sys_callback, ucallback, udata);
  if (Z_MALLOC_IS_NULL(call)) {
    return(-1);
  }

  z_fifobuf_open(&writer);
  r = z_fifobuf_reserve(&writer, Z_IPC_MSG_HEAD);
  if (Z_UNLIKELY(r != 0)) return(-1);

  /* Write the RPC header */
  r = z_rpc_write_head(head, ctx->msg_type, ctx->req_id, 1);
  r = z_fifobuf_add(&writer, head, r);
  if (Z_UNLIKELY(r != 0)) return(-1);

  /* Write the Response */
  switch (ctx->msg_type) {
{REQ_HANDLE}

    default:
      Z_LOG_FATAL("Unknown type %"PRIu64" for request\\n", ctx->msg_type);
      break;
  }

  if (Z_LIKELY(r == 0)) {
    r = z_ipc_msgbuf_push(msgbuf, &writer);
    z_iopoll_set_writable(Z_IOPOLL_ENTITY(ctx->client), 1);
    Z_LOG_TRACE("Send request of size=%u time=%.5fmsec",
                writer.size, (z_time_micros() - ctx->req_time) / 1000.0f);
  }

  z_fifobuf_close(&writer);
  return(r);
}
""", rsources, rvars))

  def add_rpc_server(self, entity):
    rpc_ids = []
    rpc_methods = []
    for call in entity.calls:
      spaces = ' ' * len(call.name)
      rpc_ids.append('#define %s_ID %d' % (call.name.upper(), call.uid))
      rpc_methods.append("  int (*%s) (z_rpc_ctx_t *ctx,\n" % call.name +
                         "        %s   struct %s *req,\n" % (spaces, call.atype) +
                         "        %s   struct %s *resp);" % (spaces, call.rtype))


    req_alloc = []
    req_parse = []
    req_free = []
    req_exec = []
    req_exec_task = []
    resp_handle = []
    for call in entity.calls:
      rvars = {
        '{MSG_TYPE}': call.uid,
        '{REQ_NAME}': call.name,
        '{REQ_ATYPE}': call.atype,
        '{REQ_RTYPE}': call.rtype,
        '{REQ_TYPE_ID}': call.rtype.upper(),
        '{RESP_TYPE_ID}': call.rtype.upper(),
      }

      req_alloc.append(replace("""
    case {MSG_TYPE}: { /* {MSG_TYPE} {REQ_NAME} */
      struct {REQ_RTYPE} *resp;
      struct {REQ_ATYPE} *req;

      Z_LOG_TRACE("Handling RPC RequestId %lu Type %lu {REQ_NAME} from client %d",
                  head->req_id, head->msg_type, Z_IOPOLL_ENTITY_FD(client));

      ctx = z_rpc_ctx_alloc(struct {REQ_ATYPE}, struct {REQ_RTYPE});
      if (Z_MALLOC_IS_NULL(ctx)) {
        Z_LOG_FATAL("Unable to allocate rpc context {REQ_NAME}");
        break;
      }

      ctx->client   = Z_IPC_MSG_CLIENT(client);
      req  = z_rpc_ctx_request(ctx, {REQ_NAME});
      resp = z_rpc_ctx_response(ctx, {REQ_NAME});
      ctx->req_id   = head->req_id;
      ctx->req_time = req_st_time;
      ctx->msg_type = {MSG_TYPE};

      req = {REQ_ATYPE}_alloc(req);
      if (Z_MALLOC_IS_NULL(req)) {
        Z_LOG_FATAL("Unable to allocate request {REQ_ATYPE}");
        z_rpc_ctx_free(ctx);
        ctx = NULL;
        break;
      }

      resp = {REQ_RTYPE}_alloc(resp);
      if (Z_MALLOC_IS_NULL(resp)) {
        Z_LOG_FATAL("Unable to allocate response {REQ_RTYPE}");
        {REQ_ATYPE}_free(req);
        z_rpc_ctx_free(ctx);
        ctx = NULL;
        break;
      }
      break;
    }
""", rvars))

      req_free.append(replace("""
    case {MSG_TYPE}: { /* {MSG_TYPE} {REQ_NAME} */
      Z_LOG_TRACE("Free RPC RequestId %lu Type %lu {REQ_NAME} from client %d",
                  ctx->req_id, ctx->msg_type, Z_IOPOLL_ENTITY_FD(client));
      {REQ_ATYPE}_free(z_rpc_ctx_request(ctx, {REQ_NAME}));
      {REQ_RTYPE}_free(z_rpc_ctx_response(ctx, {REQ_NAME}));
      break;
    }
""", rvars))

      req_parse.append(replace("""
    case {MSG_TYPE}: { /* {MSG_TYPE} {REQ_NAME} */
      Z_LOG_TRACE("Parsing RPC RequestId %lu Type %lu {REQ_NAME} from client %d",
                  ctx->req_id, ctx->msg_type, Z_IOPOLL_ENTITY_FD(client));
      r = {REQ_ATYPE}_parse(z_rpc_ctx_request(ctx, {REQ_NAME}), &reader, size);
      break;
    }
""", rvars))

      if call.async:
        req_exec.append(replace("""
    case {MSG_TYPE}: { /* {MSG_TYPE} {REQ_NAME} */
      Z_LOG_TRACE("Async Exec RPC RequestId %lu Type %lu {REQ_NAME} from client %d",
                  ctx->req_id, ctx->msg_type, Z_IOPOLL_ENTITY_FD(ctx->client));
      z_task_rq_attach(&(ctx->group), user_rq);
      return(z_rpc_ctx_add_async(ctx, __{ENTITY_NAME}_rpc_task, (void *)proto));
    }
""", rvars))
      else:
        req_exec.append(replace("""
    case {MSG_TYPE}: { /* {MSG_TYPE} {REQ_NAME} */
      Z_LOG_TRACE("Exec RPC RequestId %lu Type %lu {REQ_NAME} from client %d",
                  ctx->req_id, ctx->msg_type, Z_IOPOLL_ENTITY_FD(ctx->client));
      return(proto->{REQ_NAME}(ctx, z_rpc_ctx_request(ctx, {REQ_NAME}), z_rpc_ctx_response(ctx, {REQ_NAME})));
    }
""", rvars))

      req_exec_task.append(replace("""
    case {MSG_TYPE}: { /* {MSG_TYPE} {REQ_NAME} */
      Z_LOG_TRACE("Exec RPC RequestId %lu Type %lu {REQ_NAME} from client %d",
                  task->ctx->req_id, task->ctx->msg_type, Z_IOPOLL_ENTITY_FD(task->ctx->client));
      proto->{REQ_NAME}(task->ctx, z_rpc_ctx_request(task->ctx, {REQ_NAME}), z_rpc_ctx_response(task->ctx, {REQ_NAME}));
      break;
    }
""", rvars))

      resp_handle.append(replace("""
    case {MSG_TYPE}: { /* {MSG_TYPE} {REQ_NAME} */
      {REQ_ATYPE}_free(z_rpc_ctx_request(ctx, {REQ_NAME}));
      r = {REQ_RTYPE}_write(z_rpc_ctx_response(ctx, {REQ_NAME}), &writer);
      {REQ_RTYPE}_free(z_rpc_ctx_response(ctx, {REQ_NAME}));
      break;
    }
""", rvars))

    rheaders = {
      '{RPC_METHODS}': '\n'.join(rpc_methods),
      '{RPC_IDS}': '\n'.join(rpc_ids),
    }

    rsources = {
      '{REQ_ALLOC}': '\n'.join(req_alloc),
      '{REQ_FREE}': '\n'.join(req_free),
      '{REQ_PARSE}': '\n'.join(req_parse),
      '{REQ_EXEC}': '\n'.join(req_exec),
      '{REQ_EXEC_TASK}': '\n'.join(req_exec_task),
      '{RESP_HANDLE}':  '\n'.join(resp_handle),
    }

    rvars = {
      '{ENTITY_NAME}': entity.name,
      '{ENTITY_NAME_UP}': entity.name.upper(),
    }

    self.headers_server.append(replace("""
struct {ENTITY_NAME}_server {
{RPC_METHODS}
};

{RPC_IDS}

void *{ENTITY_NAME}_server_msg_alloc     (z_ipc_msg_client_t *client, z_ipc_msg_head_t *msg_head);
void  {ENTITY_NAME}_server_msg_free      (z_ipc_msg_client_t *client, void *vctx);
int   {ENTITY_NAME}_server_msg_parse     (z_ipc_msg_client_t *client, void *vctx, z_memslice_t *buffer);
int   {ENTITY_NAME}_server_exec_request  (z_rpc_ctx_t *ctx,
                                          z_task_rq_t *user_rq,
                                          const struct {ENTITY_NAME}_server *proto);
int   {ENTITY_NAME}_server_push_response (void *vctx);
""", rheaders, rvars))

    self.sources_server.append(replace("""
void *{ENTITY_NAME}_server_msg_alloc (z_ipc_msg_client_t *client, z_ipc_msg_head_t *head) {
  z_rpc_ctx_t *ctx = NULL;
  uint64_t req_st_time;

  req_st_time = z_time_micros();
  switch (head->msg_type) {
{REQ_ALLOC}

    default:
      Z_LOG_FATAL("Unknown Type %"PRIu64" for RequestId %"PRIu64,
                  head->msg_type, head->req_id);
      break;
  }

  return(ctx);
}

void {ENTITY_NAME}_server_msg_free (z_ipc_msg_client_t *client, void *vctx) {
  z_rpc_ctx_t *ctx = Z_RPC_CTX(vctx);
  switch (ctx->msg_type) {
{REQ_FREE}

    default:
      Z_LOG_FATAL("Unknown type %"PRIu64" for request %"PRIu64,
                  ctx->msg_type, ctx->req_id);
      break;
  }
  z_rpc_ctx_free(ctx);
}

int {ENTITY_NAME}_server_msg_parse (z_ipc_msg_client_t *client,
                                    void *vctx,
                                    z_memslice_t *buffer)
{
  z_rpc_ctx_t *ctx = Z_RPC_CTX(vctx);
  z_iovec_reader_t reader;
  struct iovec iov[1];
  size_t size;
  int r = -1;

  iov[0].iov_base = buffer->data;
  iov[0].iov_len  = buffer->size;
  z_iovec_reader_open(&reader, iov, 1);
  size = buffer->size;

  switch (ctx->msg_type) {
{REQ_PARSE}

    default:
      Z_LOG_FATAL("Unknown type %u for request %u", ctx->msg_type, ctx->req_id);
      break;
  }

  size = buffer->size - z_reader_available(&reader);
  buffer->data += size;
  buffer->size -= size;
  z_iovec_reader_close(&reader);
  return(r);
}

z_task_rstate_t __{ENTITY_NAME}_rpc_task (z_task_t *vtask) {
  z_rpc_task_t *task = z_container_of(vtask, z_rpc_task_t, vtask);
  const struct {ENTITY_NAME}_server *proto = Z_CONST_CAST(struct {ENTITY_NAME}_server, task->data);

  switch (task->ctx->msg_type) {
{REQ_EXEC_TASK}

    default:
      Z_LOG_FATAL("Unknown type %"PRIu64" for request %"PRIu64,
                  task->ctx->msg_type, task->ctx->req_id);
      break;
  }

  z_rpc_task_free(vtask);
  return(Z_TASK_COMPLETED);
}

int {ENTITY_NAME}_server_exec_request (z_rpc_ctx_t *ctx, z_task_rq_t *user_rq,
                                       const struct {ENTITY_NAME}_server *proto)
{
  switch (ctx->msg_type) {
{REQ_EXEC}

    default:
      Z_LOG_FATAL("Unknown type %"PRIu64" for request %"PRIu64,
                  ctx->msg_type, ctx->req_id);
      break;
  }
  return(-1);
}

int {ENTITY_NAME}_server_push_response (void *vctx) {
  z_rpc_ctx_t *ctx = Z_RPC_CTX(vctx);
  z_dbuf_writer_t writer;
  z_ipc_msg_t *msg;
  int r = -1;

  do {
    z_ipc_msg_head_t head;
    z_memory_t *memory;

    /* Setup the RPC header */
    head.req_type = 0;
    head.msg_len = 3;
    head.msg_type = ctx->msg_type;
    head.req_id = ctx->req_id;

    memory = z_global_memory();
    msg = z_ipc_msg_alloc(memory, &head);
    if (Z_UNLIKELY(msg == NULL))
      return(-1);

    z_ipc_msg_writer_open(msg, &writer, memory);
  } while (0);

  /* Write the Response */
  switch (ctx->msg_type) {
{RESP_HANDLE}

    default:
      Z_LOG_FATAL("Unknown type %"PRIu64" for request\\n", ctx->msg_type);
      break;
  }

  z_ipc_msg_writer_close(msg, &writer);
  if (Z_LIKELY(r == 0)) {
    msg->wtime = z_time_micros();
    msg->latency = msg->wtime - ctx->req_time;
    z_ipc_msgbuf_push(&(ctx->client->msgbuf), msg);
    z_iopoll_set_writable(Z_IOPOLL_ENTITY(ctx->client), 1);
    Z_LOG_TRACE("Send response of size=%u for RequestId %"PRIu64" time=%.5fmsec",
                writer.size, ctx->req_id, msg->latency / 1000.0f);
  }
  z_rpc_ctx_free(ctx);
  return(r);
}
""", rsources, rvars))

  def write(self, outdir=None):
    if self.headers:
      fd_h, fd_s = self._open_wfiles(outdir)
      self.write_header(fd_h)
      self.write_source(fd_s)

    if 0 and self.headers_client:
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
      print ' [__] %s.{h,c}' % path
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
#ifndef _{MODULE_NAME_UPPER}_GEN_H_
#define _{MODULE_NAME_UPPER}_GEN_H_

#include <zcl/field-coding.h>
#include <zcl/int-coding.h>
#include <zcl/dbuffer.h>
#include <zcl/macros.h>
#include <zcl/reader.h>
#include <zcl/writer.h>
#include <zcl/memref.h>
#include <zcl/bitmap.h>
#include <zcl/iovec.h>
#include <zcl/array.h>
#include <zcl/debug.h>

{HEADER_DATA}

#endif /* !_{MODULE_NAME_UPPER}_GEN_H_ */
""")

  def write_client_header(self, fd):
    header_vars = {
      '{MODULE_NAME}': self.module,
      '{MODULE_NAME_UPPER}': self.module.upper(),
      '{HEADER_DATA}': '\n'.join(self.headers_client)
    }

    self.write_blob(fd, header_vars, """
/* File autogenerated, do not edit */
#ifndef _{MODULE_NAME_UPPER}_CLIENT_GEN_H_
#define _{MODULE_NAME_UPPER}_CLIENT_GEN_H_

#include <zcl/debug.h>
#include <zcl/ipc.h>
#include <zcl/rpc.h>

#include "{MODULE_NAME}.h"

{HEADER_DATA}

#endif /* !_{MODULE_NAME_UPPER}_CLIENT_GEN_H_ */
""")

  def write_server_header(self, fd):
    header_vars = {
      '{MODULE_NAME}': self.module,
      '{MODULE_NAME_UPPER}': self.module.upper(),
      '{HEADER_DATA}': '\n'.join(self.headers_server)
    }

    self.write_blob(fd, header_vars, """
/* File autogenerated, do not edit */
#ifndef _{MODULE_NAME_UPPER}_SERVER_GEN_H_
#define _{MODULE_NAME_UPPER}_SERVER_GEN_H_

#include <zcl/debug.h>
#include <zcl/ipc.h>
#include <zcl/rpc.h>

#include "{MODULE_NAME}.h"

{HEADER_DATA}

#endif /* !_{MODULE_NAME_UPPER}_SERVER_GEN_H_ */
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
#include <zcl/global.h>
#include <zcl/debug.h>
#include <zcl/time.h>

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
#include <zcl/debug.h>
#include <zcl/time.h>

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
  return parse_struct_entity(name + '_request', fields)

def parse_response_entity(name, fields):
  return parse_struct_entity(name + '_response', fields)

def parse_rpc_entity(name, fields):
  entity = RpcEntity(name)
  for call_id, call_body in fields:
    call_type, call_name = call_body.split()
    async = call_type == 'async'
    entity.add_call(call_id, call_name, call_name + '_response', call_name + '_request', async)
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

  def add_field(self, uid, vtype, name, default):
    if len(self.fields) >= 128:
      raise Exception("Max number of field reached")
    self.fields.append(self.Field(uid, vtype, name, default))

  def guess_field_length(self):
    field_length = 0
    for field in self.fields:
      if field in PRIMITIVE_TYPES_MAX_SIZE:
        return 8
      length = PRIMITIVE_TYPES_MAX_SIZE[field.vtype]
      field_length += z_field_encoded_length(field.uid, length)
    return z_uint64_size(field_length)

  def __repr__(self):
    return 'struct %s {\n  %s\n};' % (self.name, '\n  '.join(map(str, self.fields)))

class RpcEntity(object):
  etype = 'rpc'

  class Call(object):
    def __init__(self, uid, name, rtype, atype, async):
      self.uid = uid
      self.name = name
      self.rtype = rtype
      self.atype = atype
      self.async = async

    def __repr__(self):
      return '%2d: %s %s (%s);' % (self.uid, self.rtype, self.name, self.atype)

  def __init__(self, name):
    self.name = name
    self.calls = []

  def add_call(self, uid, name, rtype, atype, async):
    self.calls.append(self.Call(uid, name, rtype, atype, async))

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