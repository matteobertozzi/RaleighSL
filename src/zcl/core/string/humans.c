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

#include <ctype.h>
#include <stdio.h>
#include <time.h>

#include <zcl/humans.h>

static const char __weekday_name[7][3] = {
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static const char __month_name[12][3] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

char *z_human_dops (char *buffer, size_t bufsize, double n) {
  if (n >= 1000000000000000000ull)
    snprintf(buffer, bufsize, "%.2fE", n / 1000000000000000000ull);
  else if (n >= 1000000000000000ull)
    snprintf(buffer, bufsize, "%.2fP", n / 1000000000000000ull);
  else if (n >= 1000000000000ull)
    snprintf(buffer, bufsize, "%.2fT", n / 1000000000000ull);
  else if (n >= 1000000000u)
    snprintf(buffer, bufsize, "%.2fG", n / 1000000000u);
  else if (n >= 1000000u)
    snprintf(buffer, bufsize, "%.2fM", n / 1000000u);
  else if (n >= Z_KB(1ULL))
    snprintf(buffer, bufsize, "%.2fK", n / 1000u);
  else
    snprintf(buffer, bufsize, "%.0f", n);
  return(buffer);
}

char *z_human_dsize (char *buffer, size_t bufsize, double size) {
  if (size >= Z_EB(1ULL))
    snprintf(buffer, bufsize, "%.2fEiB", size / Z_EB(1ULL));
  else if (size >= Z_PB(1ULL))
    snprintf(buffer, bufsize, "%.2fPiB", size / Z_PB(1ULL));
  else if (size >= Z_TB(1ULL))
    snprintf(buffer, bufsize, "%.2fTiB", size / Z_TB(1ULL));
  else if (size >= Z_GB(1ULL))
    snprintf(buffer, bufsize, "%.2fGiB", size / Z_GB(1ULL));
  else if (size >= Z_MB(1ULL))
    snprintf(buffer, bufsize, "%.2fMiB", size / Z_MB(1ULL));
  else if (size >= Z_KB(1ULL))
    snprintf(buffer, bufsize, "%.2fKiB", size / Z_KB(1ULL));
  else
    snprintf(buffer, bufsize, "%.0fbytes", size);
  return(buffer);
}

char *z_human_dtime (char *buffer, size_t bufsize, double usec) {
  if (usec >= 60000000)
    snprintf(buffer, bufsize, "%.2fmin", usec / 60000000.0);
  else if (usec >= 1000000)
    snprintf(buffer, bufsize, "%.2fsec", usec / 1000000.0);
  else if (usec >= 1000U)
    snprintf(buffer, bufsize, "%.2fmsec", usec / 1000.0);
  else
    snprintf(buffer, bufsize, "%.2fusec", usec);
  return(buffer);
}

char *z_human_ops (char *buffer, size_t bufsize, uint64_t n) {
  return z_human_dops(buffer, bufsize, n);
}

char *z_human_size (char *buffer, size_t bufsize, uint64_t size) {
  return(z_human_dsize(buffer, bufsize, size));
}

char *z_human_time (char *buffer, size_t bufsize, uint64_t usec) {
  return(z_human_dtime(buffer, bufsize, usec));
}

char *z_human_date (char *buffer, size_t bufsize, uint64_t usec) {
  struct tm datetime;
  time_t t;

  t = usec / 1000000;
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
    if (!isalnum(*pdata)) {
      use_hex = 1;
      break;
    }
  }

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

int z_human_dblock_print (FILE *stream, const void *data, size_t dsize) {
  const uint8_t *ebuf = Z_CONST_CAST(uint8_t, data + dsize);
  const uint8_t *pbuf;
  int use_hex = 0;
  int count;

  pbuf = Z_CONST_CAST(uint8_t, data);
  for (; pbuf < ebuf; ++pbuf) {
    if (!isalnum(*pbuf)) {
      use_hex = 1;
      break;
    }
  }

  use_hex = 1;
  pbuf = Z_CONST_CAST(uint8_t, data);
  count = fprintf(stream, "%zu:", dsize);
  if (use_hex) {
    count += fprintf(stream, "0x");
    for (; pbuf < ebuf; ++pbuf) {
      count += fprintf(stream, "%02x", *pbuf);
    }
  } else {
    for (; pbuf < ebuf; ++pbuf) {
      count += fprintf(stream, "%c", *pbuf);
    }
  }
  return(count);
}
