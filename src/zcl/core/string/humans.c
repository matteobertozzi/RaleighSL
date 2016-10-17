/*
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include <zcl/humans.h>
#include <zcl/time.h>

#include <stdio.h>
#include <ctype.h>
#include <time.h>

static const char __weekday_name[7][3] = {
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static const char __month_name[12][3] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

char *z_human_ops_d (char *buffer, size_t bufsize, double n) {
  if (n >= 1000000000000000000ull) {
    snprintf(buffer, bufsize, "%.2fE", n / 1000000000000000000.0);
  } else if (n >= 1000000000000000ull) {
    snprintf(buffer, bufsize, "%.2fP", n / 1000000000000000.0);
  } else if (n >= 1000000000000ull) {
    snprintf(buffer, bufsize, "%.2fT", n / 1000000000000.0);
  } else if (n >= 1000000000u) {
    snprintf(buffer, bufsize, "%.2fG", n / 1000000000.0);
  } else if (n >= 1000000u) {
    snprintf(buffer, bufsize, "%.2fM", n / 1000000.0);
  } else if (n >= 1000u) {
    snprintf(buffer, bufsize, "%.2fK", n / 1000.0);
  } else {
    snprintf(buffer, bufsize, "%.0f", n);
  }
  return(buffer);
}

char *z_human_size_d (char *buffer, size_t bufsize, double size) {
  if (size >= Z_EIB(1ull)) {
    snprintf(buffer, bufsize, "%.2fEiB", size / Z_EIB(1ull));
  } else if (size >= Z_PIB(1ull)) {
    snprintf(buffer, bufsize, "%.2fPiB", size / Z_PIB(1ull));
  } else if (size >= Z_TIB(1ull)) {
    snprintf(buffer, bufsize, "%.2fTiB", size / Z_TIB(1ull));
  } else if (size >= Z_GIB(1ull)) {
    snprintf(buffer, bufsize, "%.2fGiB", size / Z_GIB(1ull));
  } else if (size >= Z_MIB(1ull)) {
    snprintf(buffer, bufsize, "%.2fMiB", size / Z_MIB(1ull));
  } else if (size >= Z_KIB(1ull)) {
    snprintf(buffer, bufsize, "%.2fKiB", size / Z_KIB(1ull));
  } else {
    snprintf(buffer, bufsize, "%.0fbytes", size);
  }
  return(buffer);
}

char *z_human_time_d (char *buffer, size_t bufsize, double nsec) {
  if (nsec >= Z_TIME_MINUTES_TO_NANOS(1)) {
    snprintf(buffer, bufsize, "%.2fmin", Z_TIME_NANOS_TO_MINUTES_F(nsec));
  } else if (nsec >= Z_TIME_SECONDS_TO_NANOS(1)) {
    snprintf(buffer, bufsize, "%.2fsec", Z_TIME_NANOS_TO_SECONDS_F(nsec));
  } else if (nsec >= Z_TIME_MILLIS_TO_NANOS(1)) {
    snprintf(buffer, bufsize, "%.2fmsec", Z_TIME_NANOS_TO_MILLIS_F(nsec));
  } else if (nsec >= Z_TIME_MICROS_TO_NANOS(1)) {
    snprintf(buffer, bufsize, "%.2fusec", Z_TIME_NANOS_TO_MICROS_F(nsec));
  } else {
    snprintf(buffer, bufsize, "%.2fnsec", nsec);
  }
  return(buffer);
}

char *z_human_ops (char *buffer, size_t bufsize, uint64_t n) {
  return z_human_ops_d(buffer, bufsize, n);
}

char *z_human_size (char *buffer, size_t bufsize, uint64_t size) {
  return z_human_size_d(buffer, bufsize, size);
}

char *z_human_time (char *buffer, size_t bufsize, uint64_t nsec) {
  return z_human_time_d(buffer, bufsize, nsec);
}

char *z_human_date (char *buffer, size_t bufsize, uint64_t usec) {
  struct tm datetime;
  time_t t;

  t = Z_TIME_MICROS_TO_SECONDS(usec);
  localtime_r(&t, &datetime);

  snprintf(buffer, bufsize, "%.3s %.3s %.2d %d %.2d:%.2d:%.2d ",
           __weekday_name[datetime.tm_wday],
           __month_name[datetime.tm_mon],
           datetime.tm_mday, 1900 + datetime.tm_year,
           datetime.tm_hour, datetime.tm_min, datetime.tm_sec);
  return(buffer);
}

char *z_human_dblock (char *buffer, size_t bufsize, const void *data, size_t dsize) {
  const uint8_t *edata = Z_CONST_CAST(uint8_t, data + dsize);
  const uint8_t *pdata;
  char *pbuf = buffer;
  int use_hex = 0;
  int n;

  pdata = Z_CONST_CAST(uint8_t, data);
  for (; pdata < edata; ++pdata) {
    if (!isprint(*pdata)) {
      use_hex = 1;
      break;
    }
  }

  pdata = Z_CONST_CAST(uint8_t, data);
  n = snprintf(pbuf, bufsize, "%zu:", dsize);
  bufsize -= n;
  pbuf += n;

  if (use_hex) {
    n = snprintf(pbuf, bufsize, "0x");
    bufsize -= n;
    pbuf += n;

    for (; bufsize > 0 && pdata < edata; ++pdata) {
      n = snprintf(pbuf, bufsize, "%02x", *pdata);
      bufsize -= n;
      pbuf += n;
    }
  } else {
    for (; bufsize > 0 && pdata < edata; ++pdata) {
      n = snprintf(pbuf, bufsize, "%c", *pdata);
      bufsize -= n;
      pbuf += n;
    }
  }
  return(buffer);
}
