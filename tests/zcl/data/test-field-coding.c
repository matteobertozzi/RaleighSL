#include <zcl/field-coding.h>
#include <zcl/debug.h>
#include <zcl/time.h>

#include <stdio.h>

#define FIRST_I      (0)
#define LAST_I       (256 << 20)
#define STEP_I       (1)
#define NITEMS       (LAST_I - FIRST_I)

int main (int argc, char **argv) {
  uint64_t i;

  Z_DEBUG_TOPS(FIELD_ENCODE, 1000 * (1 << 16), {
    uint32_t j;
    for (j = 0; j < 1000; ++j) {
      for (i = 0; i < (1 << 16); ++i) {
        uint8_t buffer[16];
        z_field_encode(buffer, i, i * 101);
      }
    }
  });

  Z_DEBUG_TOPS(FIELD_ENC_DEC, 1000 * (1 << 16), {
    uint32_t j;
    for (j = 0; j < 1000; ++j) {
      for (i = 0; i < (1 << 16); ++i) {
        uint8_t buffer[16];
        uint64_t length;
        uint16_t field_id;
        uint8_t len;
        len = z_field_encode(buffer, i, i * 101);
        z_field_decode(buffer, len, &field_id, &length);
      }
    }
  });

  Z_DEBUG_TOPS(FIELD_LENGTH, 1000 * (1 << 16), {
    uint32_t j;
    for (j = 0; j < 1000; ++j) {
      for (i = 0; i < (1 << 16); ++i) {
        uint8_t x = z_field_encoded_length(i, i * 101);
        x = 0;
      }
    }
  });
  return 0;
}
