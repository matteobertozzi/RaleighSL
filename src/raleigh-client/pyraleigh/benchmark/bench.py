from raleigh.client import RaleighClient, RaleighException
from raleigh.iopoll import IOPoll
from time import time
from random import randint

def _job_func(job_cls, oid):
  try:
    return job_cls(oid).run()
  except KeyboardInterrupt as e:
    return None, None, e
  except Exception as e:
    return None, None, e

def bench_run(nthread, job_cls, oid):
  from multiprocessing import Pool
  pool = Pool(nthread)
  results = []
  for _ in xrange(nthread):
    r = pool.apply_async(_job_func, (job_cls, oid))
    results.append(r)

  total_time = 0
  total_requests = 0
  for r in results:
    jtime, jreqs, exception = r.get()
    if jtime is not None:
      print '- Job Finished %r in %.3fsec (%.3freq/sec)' % (exception, jtime, jreqs / jtime)
      total_time += jtime
      total_requests += jreqs
    else:
      print exception
  print ' --> Total time %3fsec (%.3freq/sec)' % (total_time, (total_requests * nthread) / total_time)

class _RaleighJob(object):
  RALEIGH_HOST = '127.0.0.1'
  RALEIGH_PORT = 11215
  NREQUESTS = 10000

  def __init__(self, oid):
    super(_RaleighJob, self).__init__()
    self.client = None
    self.oid = oid

  def run(self):
    iopoll = IOPoll()
    with iopoll.loop():
      client = RaleighClient()
      client.REPLY_MAX_WAIT = 20
      with client.connection(iopoll, self.RALEIGH_HOST, self.RALEIGH_PORT):
        exception = None
        nrequests = 0
        st = time()
        try:
          nrequests, exception = self.execute(client)
        except Exception as e:
          exception = e
        finally:
          et = time()
        return (et - st, nrequests, exception)

  def execute(self, client):
    raise NotImplementedError
