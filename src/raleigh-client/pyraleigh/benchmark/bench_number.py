from raleigh.client import RaleighClient, RaleighException
from raleigh.objects import RaleighNumber
from raleigh.iopoll import IOPoll

from bench import _RaleighJob, bench_run
from time import time

class NumberIncJob(_RaleighJob):
  def execute(self, client):
    number = RaleighNumber(client, self.oid)
    nrequests = 0
    exception = None
    try:
      for i in xrange(self.NREQUESTS):
        nrequests += 1
        number.inc()
    except Exception as e:
      exception = e
    return nrequests, exception

class NumberGetJob(_RaleighJob):
  def execute(self, client):
    number = RaleighNumber(client, self.oid)
    nrequests = 0
    exception = None
    try:
      for i in xrange(self.NREQUESTS):
        nrequests += 1
        data = number.get()
    except Exception as e:
      exception = e
    return nrequests, exception

def main():
  iopoll = IOPoll()
  with iopoll.loop():
    NTHREADS = 8
    client = RaleighClient()
    with client.connection(iopoll, _RaleighJob.RALEIGH_HOST, _RaleighJob.RALEIGH_PORT):
      object_name = 'bench-number'
      data = client.semantic_create(object_name, RaleighNumber.TYPE)
      try:
        print 'Bench Number-Insert'
        bench_run(NTHREADS, NumberIncJob, data['oid'])
        print 'Bench Number-Get'
        bench_run(NTHREADS, NumberGetJob, data['oid'])
      finally:
        client.semantic_delete(object_name)

if __name__ == '__main__':
  main()
