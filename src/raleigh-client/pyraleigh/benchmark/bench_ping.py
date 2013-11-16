from raleigh.client import RaleighClient
from raleigh.iopoll import IOPoll

from bench import _RaleighJob, bench_run

class PingJob(_RaleighJob):
  def execute(self, client):
    nrequests = 0
    exception = None
    for i in xrange(self.NREQUESTS):
      try:
        nrequests += 1
        client.ping()
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
      print 'Bench Ping'
      bench_run(NTHREADS, PingJob, None)

if __name__ == '__main__':
  main()
