#ifndef _RPC_STATS_H_
#define _RPC_STATS_H_

#include <zcl/macros.h>
#include <zcl/coding.h>
#include <zcl/reader.h>

struct rpc_rusage {
   /* Internal states */
   uint8_t    rusage_fields_bitmap[2];

   /* Fields */
   uint64_t   utime;                       /*  1: uint64 None */
   uint64_t   stime;                       /*  2: uint64 None */
   uint64_t   maxrss;                      /*  3: uint64 None */
   uint64_t   minflt;                      /*  4: uint64 None */
   uint64_t   majflt;                      /*  5: uint64 None */
   uint64_t   inblock;                     /*  6: uint64 None */
   uint64_t   oublock;                     /*  7: uint64 None */
   uint64_t   nvcsw;                       /*  8: uint64 None */
   uint64_t   nivcsw;                      /*  9: uint64 None */
   uint64_t   iowait;                      /* 10: uint64 None */
   uint64_t   ioread;                      /* 11: uint64 None */
   uint64_t   iowrite;                     /* 12: uint64 None */
};

#define rpc_rusage_has_utime(msg)               z_test_bit((msg)->rusage_fields_bitmap, 0)
#define rpc_rusage_has_stime(msg)               z_test_bit((msg)->rusage_fields_bitmap, 1)
#define rpc_rusage_has_maxrss(msg)              z_test_bit((msg)->rusage_fields_bitmap, 2)
#define rpc_rusage_has_minflt(msg)              z_test_bit((msg)->rusage_fields_bitmap, 3)
#define rpc_rusage_has_majflt(msg)              z_test_bit((msg)->rusage_fields_bitmap, 4)
#define rpc_rusage_has_inblock(msg)             z_test_bit((msg)->rusage_fields_bitmap, 5)
#define rpc_rusage_has_oublock(msg)             z_test_bit((msg)->rusage_fields_bitmap, 6)
#define rpc_rusage_has_nvcsw(msg)               z_test_bit((msg)->rusage_fields_bitmap, 7)
#define rpc_rusage_has_nivcsw(msg)              z_test_bit((msg)->rusage_fields_bitmap, 8)
#define rpc_rusage_has_iowait(msg)              z_test_bit((msg)->rusage_fields_bitmap, 9)
#define rpc_rusage_has_ioread(msg)              z_test_bit((msg)->rusage_fields_bitmap, 10)
#define rpc_rusage_has_iowrite(msg)             z_test_bit((msg)->rusage_fields_bitmap, 11)

void rpc_rusage_parse (struct rpc_rusage *msg, void *reader);
uint8_t *rpc_rusage_write_utime (uint8_t *buffer, uint64_t value);
uint8_t *rpc_rusage_write_stime (uint8_t *buffer, uint64_t value);
uint8_t *rpc_rusage_write_maxrss (uint8_t *buffer, uint64_t value);
uint8_t *rpc_rusage_write_minflt (uint8_t *buffer, uint64_t value);
uint8_t *rpc_rusage_write_majflt (uint8_t *buffer, uint64_t value);
uint8_t *rpc_rusage_write_inblock (uint8_t *buffer, uint64_t value);
uint8_t *rpc_rusage_write_oublock (uint8_t *buffer, uint64_t value);
uint8_t *rpc_rusage_write_nvcsw (uint8_t *buffer, uint64_t value);
uint8_t *rpc_rusage_write_nivcsw (uint8_t *buffer, uint64_t value);
uint8_t *rpc_rusage_write_iowait (uint8_t *buffer, uint64_t value);
uint8_t *rpc_rusage_write_ioread (uint8_t *buffer, uint64_t value);
uint8_t *rpc_rusage_write_iowrite (uint8_t *buffer, uint64_t value);

#endif /* _RPC_STATS_H_ */
