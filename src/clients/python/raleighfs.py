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

from netclient import NetIOClientWrapper
import coding

class IpcFramedClient(NetIOClientWrapper):
    def send(self, data):
        buf = str(coding.z_encode_vint(len(data))) + data
        return self._client.send(buf)

    def recv(self):
        ibuf = self._client.fetch()
        while len(ibuf) > 0:
            #elen, field_id, length = coding.z_decode_field(ibuf)
            elen, length = coding.z_decode_vint(ibuf)
            if elen < 0: break
            data = self._client.pop(elen + length)
            yield data[elen:]
            ibuf = self._client.fetch()

class IpcRpcClient(IpcFramedClient):
    def send_message(self, msg_type, data):
        msghead = coding.z_encode_field_uint(1, msg_type)
        self.send(msghead + data)

class RaleighFS(IpcRpcClient):
    pass

class StatsClient(IpcRpcClient):
    def rusage(self):
        self.send_message(1, 'x')

    def recv_struct(self):
        for data in self.recv():
            yield RUsageStruct.parse(data)

class FieldStruct(object):
    _UNKNOWN_FIELD = (None, None, None)
    _FIELDS = {}

    def __init__(self):
        self._values = {}

    def keys(self):
        return self._values.iterkeys()

    def __repr__(self):
        return str(self._values)

    def __getattr__(self, name):
        value = self._values.get(name)
        if value is None:
            raise AttributeError(name)
        return value

    @classmethod
    def parse(cls, data):
        self = cls()
        while len(data) > 0:
            elength, field, length = coding.z_decode_field(data)
            data = data[elength:]
            field_name, field_type, _ = self._FIELDS.get(field, self._UNKNOWN_FIELD)
            if field_name is not None and field_type is not None:
                func = getattr(coding, 'z_decode_' + field_type)
                self._values[field_name] = func(data, length)
            data = data[length:]
        return self

class RUsageStruct(FieldStruct):
    _FIELDS = {
         1: ('utime',        'uint',  None),
         2: ('stime',        'uint',  None),
         3: ('maxrss',       'uint',  None),
         4: ('minflt',       'uint',  None),
         5: ('majflt',       'uint',  None),
         6: ('inblock',      'uint',  None),
         7: ('oublock',      'uint',  None),
         8: ('nvcsw',        'uint',  None),
         9: ('nivcsw',       'uint',  None),
        10: ('iowait',       'uint',  None),
        11: ('ioread',       'uint',  None),
        12: ('iowrite',      'uint',  None),
    }

class MsgHeadStruct(FieldStruct):
    _FIELDS = {
         0: ('req_id',       'uint',  None),
         1: ('req_type',     'uint',  None),
    }

