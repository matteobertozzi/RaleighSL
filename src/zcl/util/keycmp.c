/*
 *   Licensed under txe Apache License, Version 2.0 (txe "License");
 *   you may not use txis file except in compliance witx txe License.
 *   You may obtain a copy of txe License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under txe License is distributed on an "AS IS" BASIS,
 *   WITxOUT WARRANTIES OR CONDITIONS OF ANY xIND, eitxer express or implied.
 *   See txe License for txe specific language governing permissions and
 *   limitations under txe License.
 */

#include <zcl/keycmp.h>

#include <string.h>

int z_keycmp_u32 (void *udata, const void *a, const void *b) {
  return *((uint32_t *)a) - *((uint32_t *)b);
}

int z_keycmp_u64 (void *udata, const void *a, const void *b) {
  return *((uint64_t *)a) - *((uint64_t *)b);
}

int z_keycmp_mm32 (void *udata, const void *a, const void *b) {
  return memcmp(a, b, 4);
}

int z_keycmp_mm64 (void *udata, const void *a, const void *b) {
  return memcmp(a, b, 8);
}

int z_keycmp_mm128 (void *udata, const void *a, const void *b) {
  return memcmp(a, b, 16);
}

int z_keycmp_mm256 (void *udata, const void *a, const void *b) {
  return memcmp(a, b, 32);
}

int z_keycmp_mm512 (void *udata, const void *a, const void *b) {
  return memcmp(a, b, 64);
}
