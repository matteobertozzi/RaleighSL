#include <string.h>
#include <signal.h>
#include <stdio.h>

#include <zcl/eloop.h>
#include <zcl/debug.h>

struct signal_data {
  uint64_t nevents;
  uint64_t stime;
  int term_signum;
};

static z_event_loop_t __eloop[1];

static struct signal_data __signal_data;
static void __signal_handler (int signum) {
  __signal_data.term_signum = signum;
  z_event_loop_stop(__eloop + 0);
}

static int __test_eloop_exec (z_event_loop_t *eloop) {
  z_eloop_channel_notify(eloop);
  return(0);
}

static int __test_eloop_event (z_event_loop_t *eloop) {
  __signal_data.nevents++;
  return(0);
}

static int __test_eloop_timeout (z_event_loop_t *eloop) {
  char buffer[64];
  float xload = z_iopoll_engine_load(&(eloop->iopoll));
  float sec = (z_time_micros() - __signal_data.stime) / 1000000.0f;
  z_human_dops(buffer, sizeof(buffer), __signal_data.nevents / sec);
  Z_DEBUG("%d NEVENTS %.3fsec %s/sec (I/O ACTIVE %.2f%% IDLE %.2f%%)",
          eloop->core, sec, buffer, xload, 100 - xload);
  if (Z_UNLIKELY(sec > 10)) {
    z_event_loop_stop(__eloop + 0);
  }
  return(0);
}

const z_event_loop_vtable_t __eloop_test_vtable = {
  .exec     = __test_eloop_exec,
  .event    = __test_eloop_event,
  .timeout  = __test_eloop_timeout,
};


int main (int argc, char **argv) {
  /* Initialize signals */
  __signal_data.nevents = 0;
  __signal_data.term_signum = 0;
  signal(SIGINT,  __signal_handler);
  signal(SIGTERM, __signal_handler);

  z_debug_open();
  z_event_loop_open(__eloop + 0, &__eloop_test_vtable, 1);

  z_eloop_channel_set_timer(__eloop + 0, 2000);
  __signal_data.stime = z_time_micros();
  z_event_loop_start(__eloop + 0, 1);
  z_event_loop_wait(__eloop + 0);

  z_event_loop_close(__eloop + 0);
  z_event_loop_dump(__eloop + 0, stderr);
  z_debug_close();

  fprintf(stderr, "terminated with signum %d\n", __signal_data.term_signum);
  return(0);
}
