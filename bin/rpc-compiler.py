#!/usr/bin/env python

import re

def fnv1_hash(word):
  h = 0xcbf29ce484222325
  for b in word:
    h ^= ord(b)
    h *= 0x100000001b3
    h %= 1 << 64
  return h

class Field(object):
    def __init__(self, uid, vtype, name, default=None):
        if uid < 0 or uid >= 64:
            raise Exception("Invalid field uid: %d" % uid)

        self.default = default
        self.vtype = vtype
        self.name = name
        self.uid = uid

    def __repr__(self):
        s = '%2d: %s %s' % (self.uid, self.vtype, self.name)
        if self.default is not None:
            s += ' [%s]' % self.default
        return s + ';'

class Message(object):
    def __init__(self, mtype, name):
        self.name = name
        self.mtype = mtype
        self.fields = []

    def add_field(self, uid, vtype, name, default):
        if len(self.fields) >= 128:
            raise Exception("Max number of field reached")
        self.fields.append(Field(uid, vtype, name, default))

    def __repr__(self):
        return '%s %s {\n  %s\n}' % (self.mtype, self.name, '\n  '.join(map(str, self.fields)))

    @staticmethod
    def parse(data):
        rx_message = re.compile('\s*(request|response|struct)\s+(\w+)\s*\\{([^}]*)\\}', re.MULTILINE)
        rx_field = re.compile('\s*([0-9]+)\s*:\s*(\w+)\s+(\w+)\s*(\\[.*\\])*\s*;{1,}', re.MULTILINE)

        messages = []
        for m in rx_message.finditer(data):
            m_type, m_name, m_def = m.groups()
            msg = Message(m_type, m_name)
            for m in rx_field.finditer(m_def):
                field_id, field_type, field_name, default = m.groups()
                if default is not None: default = default[1:-1].split('=')[1].strip()
                msg.add_field(int(field_id), field_type, field_name, default)
            messages.append(msg)
        return messages

    @staticmethod
    def parse_file(filename):
        fd = open(filename)
        try:
            return Message.parse(fd.read())
        finally:
            fd.close()


def py_generator(msg, prefix, out_dir):
    print 'class %s%s:' % (msg.mtype.capitalize(), msg.name.capitalize())
    print '  def __init__(self):'
    for field in msg.fields:
        print '      self.%s = %s' % (field.name, field.default)
    print

align_up = lambda x, align: ((x + (align - 1)) & (-align))
ceil = lambda a, b: ((a + b - 1) / b)
spacing = lambda space, data: ' ' * (space - len(data))

C_INT_TYPES_MAP = {
    'int8': 'int8_t', 'int16': 'int16_t', 'int32': 'int32_t', 'int64': 'int64_t',
    'uint8': 'uint8_t', 'uint16': 'uint16_t', 'uint32': 'uint32_t', 'uint64': 'uint64_t',
}

C_TYPES_MAP = {
    'bytes': 'z_slice_t',
}

def c_type(vtype):
    ctype = C_INT_TYPES_MAP.get(vtype)
    if ctype is not None: return ctype
    return C_TYPES_MAP.get(vtype, vtype)

def c_type_const_arg(vtype):
    vtype = c_type(vtype)
    if vtype in ('uint8_t', 'uint16_t', 'uint32_t', 'uint64_t'):
        return vtype
    return 'const %s *' % vtype

def c_generator(msg, prefix, out_dir, name):
    fd_h = open(os.path.join(out_dir, '%s.h' % name), 'w')
    fd_c = open(os.path.join(out_dir, '%s.c' % name), 'w')
    fd_h_write = lambda x='': fd_h.write(x + '\n')
    fd_c_write = lambda x='': fd_c.write(x + '\n')

    c_msg_type = '%s%s' % (prefix, msg.name)
    fd_h_write('#ifndef _%s%s_H_' % (prefix.upper(), name.upper()))
    fd_h_write('#define _%s%s_H_' % (prefix.upper(), name.upper()))
    fd_h_write()
    fd_h_write('#include <zcl/macros.h>')
    fd_h_write('#include <zcl/coding.h>')
    fd_h_write('#include <zcl/reader.h>')
    fd_h_write()
    fd_h_write('struct %s {' % (c_msg_type))
    fd_h_write('   /* Internal states */')
    fd_h_write('   uint8_t    %s_fields_bitmap[%d];' % (msg.name, ceil(len(msg.fields), 8)))
    fd_h_write()
    fd_h_write('   /* Fields */')
    for field in msg.fields:
        fd_h_write('   %-10s %s;%s/* %2d: %s %s */' % (c_type(field.vtype), field.name, spacing(28, field.name), field.uid, field.vtype, field.default))
    fd_h_write('};')
    fd_h_write()
    for fid, field in enumerate(msg.fields):
        macro = '%s_has_%s(msg)' % (c_msg_type, field.name)
        fd_h_write('#define %s %s z_test_bit((msg)->%s_fields_bitmap, %d)' % (macro, spacing(38, macro), msg.name, fid))
    fd_h_write()
    fd_h_write('void %s_parse (struct %s *msg, void *reader);' % (c_msg_type, c_msg_type))
    for field in msg.fields:
        fd_h_write('uint8_t *%s_write_%s (uint8_t *buffer, %s value);' % (c_msg_type, field.name, c_type(field.vtype)))
    fd_h_write()
    fd_h_write('#endif /* _%s%s_H_ */' % (prefix.upper(), name.upper()))

    fd_c_write()
    fd_c_write('#include "%s.h"' % name)
    fd_c_write()
    fd_c_write('void %s_parse (struct %s *msg, void *reader) {' % (c_msg_type, c_msg_type))
    fd_c_write('    uint16_t field_id;')
    fd_c_write('    uint64_t length;')
    fd_c_write()
    fd_c_write('    while (z_reader_decode_field(reader, &field_id, &length) > 0) {')
    fd_c_write('        switch (field_id) {')
    for field in msg.fields:
        fd_c_write('            case %d: /* %s %s */' % (field.uid, field.vtype, field.name))
        fd_c_write('                z_reader_decode_%s(reader, length, &(msg->%s));' % (field.vtype, field.name))
        fd_c_write('                break;')
    fd_c_write('            default:')
    fd_c_write('                z_reader_skip(reader, length);')
    fd_c_write('                break;')
    fd_c_write('        }')
    fd_c_write('    }')
    fd_c_write('}')
    fd_c_write()
    for field in msg.fields:
        fd_c_write('uint8_t *%s_write_%s (uint8_t *buf, %s value) {' % (c_msg_type, field.name, c_type(field.vtype)))
        fd_c_write('    unsigned length = z_uint64_bytes(value);')
        fd_c_write('    int elen = z_encode_field(buf, %d, length); buf += elen;' % (field.uid))
        fd_c_write('    z_encode_uint(buf, length, value); buf += length;')
        fd_c_write('    return(buf);')
        fd_c_write('}')
        fd_c_write()
    fd_c_write()

PY_INT_TYPES_MAP = {
    'int8': 'int', 'int16': 'int', 'int32': 'int', 'int': 'int',
    'uint8': 'uint', 'uint16': 'uint', 'uint32': 'uint32', 'uint64': 'uint',
}

PY_TYPES_MAP = {
    'bytes': 'bytes',
}

def py_type(vtype):
    pytype = PY_INT_TYPES_MAP.get(vtype)
    if pytype is not None: return pytype
    return PY_TYPES_MAP.get(vtype, vtype)

def py_generator(msg, prefix, out_dir, name):
    py = open(os.path.join(out_dir, '%s.py' % name), 'w')
    fd_write = lambda x='': py.write(x + '\n')

    msg_type = '%s%s_%s' % (prefix, msg.name, msg.mtype)

    fd_write('class %s(FieldStruct):' % (msg_type))
    fd_write('    _FIELDS = {')
    for field in msg.fields:
        fd_write("        %2d: ('%s',%s '%s',%s  %s)," % (field.uid, field.name, spacing(12, field.name), py_type(field.vtype), spacing(5, field.vtype), field.default))
    fd_write('    }')

if __name__ == '__main__':
    import sys
    import os

    if len(sys.argv) < 4:
        print 'usage: serialize [output dir] [prefix] [message file]'
        sys.exit(1)

    out_dir = sys.argv[1]
    if not os.path.exists(out_dir): os.makedirs(out_dir)
    prefix = sys.argv[2]

    for filename in sys.argv[3:]:
        name, _ = os.path.splitext(os.path.basename(filename))
        for msg in Message.parse_file(filename):
            c_generator(msg, prefix, out_dir, name)
            py_generator(msg, prefix, out_dir, name)
