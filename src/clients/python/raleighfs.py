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

class IpcRpcClient(NetIOClientWrapper):
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

class RaleighFS(IpcRpcClient):
    pass
