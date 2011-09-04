#!/usr/bin/env python

import uuid

if __name__ == '__main__':
    x = uuid.uuid4()
    vec = ','.join([str(ord(b)) for b in x.get_bytes()])
    print x
    print '.uuid = {%s};' % vec

