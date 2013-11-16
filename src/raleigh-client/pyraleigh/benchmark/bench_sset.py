from raleigh.client import RaleighClient, RaleighException
from raleigh.objects import RaleighSSet
from raleigh.iopoll import IOPoll
from random import randint

from bench import _RaleighJob, bench_run

class SSetInsertJob(_RaleighJob):
  def execute(self, client):
    sset = RaleighSSet(client, self.oid)
    nrequests = 0
    exception = None
    for i in xrange(self.NREQUESTS):
      x = randint(0, 9999999999)
      try:
        nrequests += 1
        sset.insert('Key-%010d' % x, 'Value-%010d' % x)
      except RaleighException as e:
        pass
      except Exception as e:
        exception = e
        break
    return nrequests, exception

class SSetGetJob(_RaleighJob):
  def execute(self, client):
    sset = RaleighSSet(client, self.oid)
    nrequests = 0
    exception = None
    for i in xrange(self.NREQUESTS):
      x = randint(0, 9999999999)
      try:
        nrequests += 1
        data = sset.get('Key-%010d' % x)
      except RaleighException as e:
        pass
      except Exception as e:
        exception = e
        break
    return nrequests, exception

def main():
  iopoll = IOPoll()
  with iopoll.loop():
    NTHREADS = 8
    client = RaleighClient()
    with client.connection(iopoll, _RaleighJob.RALEIGH_HOST, _RaleighJob.RALEIGH_PORT):
      object_name = 'bench'
      data = client.semantic_create(object_name, RaleighSSet.TYPE)
      try:
        print 'Bench SSet-Insert'
        bench_run(NTHREADS, SSetInsertJob, data['oid'])
        print 'Bench SSet-Get'
        bench_run(NTHREADS, SSetGetJob, data['oid'])
      finally:
        client.semantic_delete(object_name)

if __name__ == '__main__':
  main()
