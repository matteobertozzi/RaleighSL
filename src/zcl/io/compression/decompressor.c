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

#include <zcl/compression.h>
#include <zcl/coding.h>
#include <zcl/debug.h>

void z_decompressor_open (z_decompressor_t *self,
                          const void *buffer,
                          unsigned int bufsize,
                          z_decompress_t codec)
{
  self->buffer = buffer;
  self->bufsize = bufsize;
  self->codec = codec;
  z_byte_slice_clear(&(self->body));
  z_slice_alloc(&(self->prefix));
  z_slice_alloc(&(self->suffix));
  self->iprev = 0;
}

void z_decompressor_close (z_decompressor_t *self) {
  z_slice_free(&(self->prefix));
  z_slice_free(&(self->suffix));
}

int z_decompressor_read (z_decompressor_t *self, void *dst, unsigned int size) {
  uint64_t usize = 0;
  int n;

  if (self->bufsize == 0)
    return(0);

  /* read uncompressed block size */
  n = z_decode_vint(self->buffer, self->bufsize, &usize);
  self->buffer += n;
  self->bufsize -= n;

  /* read the uncompressed data */
  n = self->codec(dst, usize, self->buffer);
  self->buffer += n;
  self->bufsize -= n;

  z_byte_slice_clear(&(self->body));
  z_slice_clear(&(self->prefix));
  z_slice_clear(&(self->suffix));
  self->iprev = 0;

  /* TODO: Add sanity check (vint, decode, ...) */
  return(usize);
}

int z_decompressor_get_bytes (z_decompressor_t *self,
                              const uint8_t *buffer,
                              unsigned int bufsize,
                              z_slice_t *current)
{
  uint32_t prefix, suffix, length, zero;
  size_t slice_len;
  int head;

  head = z_decode_4int(buffer, &prefix, &suffix, &length, &zero);

  /* Adjust the prefix */
  slice_len = z_slice_length(&(self->prefix));
  if (prefix <= slice_len) {
    z_slice_ltrim(&(self->prefix), slice_len - prefix);
  } else {
    z_slice_append(&(self->prefix), self->body.data, prefix - slice_len);
  }

  /* Adjust the suffix */
  slice_len = z_slice_length(&(self->suffix));
  if (suffix <= slice_len) {
    z_slice_ltrim(&(self->suffix), slice_len - suffix);
  } else {
    size_t n = suffix - slice_len;
    z_slice_prepend(&(self->suffix), self->body.data + (self->body.size - n), n);
  }

  z_slice_clear(current);
  z_slice_add(current, &(self->prefix));
  z_slice_append(current, buffer + head, length);
  z_slice_add(current, &(self->suffix));
  return(head + length);
}

int z_decompressor_get_int (z_decompressor_t *self,
                            const uint8_t *buffer,
                            unsigned int bufsize,
                            uint64_t *current)
{
  int n;
  n = z_decode_vint(buffer, bufsize, current);
  *current += self->iprev;
  self->iprev = *current;
  return(n);
}