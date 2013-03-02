
#include "stats.h"

void rpc_rusage_parse (struct rpc_rusage *msg, void *reader) {
    uint16_t field_id;
    uint64_t length;

    while (z_reader_decode_field(reader, &field_id, &length) > 0) {
        switch (field_id) {
            case 1: /* uint64 utime */
                z_reader_decode_uint64(reader, length, &(msg->utime));
                break;
            case 2: /* uint64 stime */
                z_reader_decode_uint64(reader, length, &(msg->stime));
                break;
            case 3: /* uint64 maxrss */
                z_reader_decode_uint64(reader, length, &(msg->maxrss));
                break;
            case 4: /* uint64 minflt */
                z_reader_decode_uint64(reader, length, &(msg->minflt));
                break;
            case 5: /* uint64 majflt */
                z_reader_decode_uint64(reader, length, &(msg->majflt));
                break;
            case 6: /* uint64 inblock */
                z_reader_decode_uint64(reader, length, &(msg->inblock));
                break;
            case 7: /* uint64 oublock */
                z_reader_decode_uint64(reader, length, &(msg->oublock));
                break;
            case 8: /* uint64 nvcsw */
                z_reader_decode_uint64(reader, length, &(msg->nvcsw));
                break;
            case 9: /* uint64 nivcsw */
                z_reader_decode_uint64(reader, length, &(msg->nivcsw));
                break;
            case 10: /* uint64 iowait */
                z_reader_decode_uint64(reader, length, &(msg->iowait));
                break;
            case 11: /* uint64 ioread */
                z_reader_decode_uint64(reader, length, &(msg->ioread));
                break;
            case 12: /* uint64 iowrite */
                z_reader_decode_uint64(reader, length, &(msg->iowrite));
                break;
            default:
                z_reader_skip(reader, length);
                break;
        }
    }
}

uint8_t *rpc_rusage_write_utime (uint8_t *buf, uint64_t value) {
    unsigned length = z_uint64_bytes(value);
    int elen = z_encode_field(buf, 1, length); buf += elen;
    z_encode_uint(buf, length, value); buf += length;
    return(buf);
}

uint8_t *rpc_rusage_write_stime (uint8_t *buf, uint64_t value) {
    unsigned length = z_uint64_bytes(value);
    int elen = z_encode_field(buf, 2, length); buf += elen;
    z_encode_uint(buf, length, value); buf += length;
    return(buf);
}

uint8_t *rpc_rusage_write_maxrss (uint8_t *buf, uint64_t value) {
    unsigned length = z_uint64_bytes(value);
    int elen = z_encode_field(buf, 3, length); buf += elen;
    z_encode_uint(buf, length, value); buf += length;
    return(buf);
}

uint8_t *rpc_rusage_write_minflt (uint8_t *buf, uint64_t value) {
    unsigned length = z_uint64_bytes(value);
    int elen = z_encode_field(buf, 4, length); buf += elen;
    z_encode_uint(buf, length, value); buf += length;
    return(buf);
}

uint8_t *rpc_rusage_write_majflt (uint8_t *buf, uint64_t value) {
    unsigned length = z_uint64_bytes(value);
    int elen = z_encode_field(buf, 5, length); buf += elen;
    z_encode_uint(buf, length, value); buf += length;
    return(buf);
}

uint8_t *rpc_rusage_write_inblock (uint8_t *buf, uint64_t value) {
    unsigned length = z_uint64_bytes(value);
    int elen = z_encode_field(buf, 6, length); buf += elen;
    z_encode_uint(buf, length, value); buf += length;
    return(buf);
}

uint8_t *rpc_rusage_write_oublock (uint8_t *buf, uint64_t value) {
    unsigned length = z_uint64_bytes(value);
    int elen = z_encode_field(buf, 7, length); buf += elen;
    z_encode_uint(buf, length, value); buf += length;
    return(buf);
}

uint8_t *rpc_rusage_write_nvcsw (uint8_t *buf, uint64_t value) {
    unsigned length = z_uint64_bytes(value);
    int elen = z_encode_field(buf, 8, length); buf += elen;
    z_encode_uint(buf, length, value); buf += length;
    return(buf);
}

uint8_t *rpc_rusage_write_nivcsw (uint8_t *buf, uint64_t value) {
    unsigned length = z_uint64_bytes(value);
    int elen = z_encode_field(buf, 9, length); buf += elen;
    z_encode_uint(buf, length, value); buf += length;
    return(buf);
}

uint8_t *rpc_rusage_write_iowait (uint8_t *buf, uint64_t value) {
    unsigned length = z_uint64_bytes(value);
    int elen = z_encode_field(buf, 10, length); buf += elen;
    z_encode_uint(buf, length, value); buf += length;
    return(buf);
}

uint8_t *rpc_rusage_write_ioread (uint8_t *buf, uint64_t value) {
    unsigned length = z_uint64_bytes(value);
    int elen = z_encode_field(buf, 11, length); buf += elen;
    z_encode_uint(buf, length, value); buf += length;
    return(buf);
}

uint8_t *rpc_rusage_write_iowrite (uint8_t *buf, uint64_t value) {
    unsigned length = z_uint64_bytes(value);
    int elen = z_encode_field(buf, 12, length); buf += elen;
    z_encode_uint(buf, length, value); buf += length;
    return(buf);
}


