/*
 *   Copyright 2011 Matteo Bertozzi
 *
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

#include <unistd.h>
#include <stdio.h>

#include <zcl/chunkq.h>
#include <zcl/memchr.h>
#include <zcl/memcpy.h>
#include <zcl/memcmp.h>
#include <zcl/strtol.h>
#include <zcl/slice.h>

typedef struct z_chunkq_search {
    const uint8_t *needle;
    const uint8_t *data;
    unsigned int needle_len;
    unsigned int data_len;
    unsigned int offset;
    unsigned int rd;
    int state;
} z_chunkq_search_t;

typedef struct z_chunkq_slice_info {
    const z_slice_t *slice;
    unsigned int available;
    unsigned int offset;
} z_chunkq_slice_info_t;

static z_chunkq_node_t *__chunkq_node_alloc (z_chunkq_t *chunkq) {
    z_chunkq_node_t *node;
    unsigned int size;

    size = sizeof(z_chunkq_node_t) + (chunkq->chunk - 1);
    if ((node = z_object_block_alloc(chunkq, z_chunkq_node_t, size)) != NULL) {
        node->next = NULL;
        node->offset = 0U;
        node->size = 0U;
    }

    return(node);
}

static void __chunkq_node_free (z_chunkq_t *chunkq,
                                z_chunkq_node_t *node)
{
    z_object_block_free(chunkq, node);
}

static z_chunkq_node_t *__chunkq_node_at_offset (z_chunkq_t *chunkq,
                                                 unsigned int offset,
                                                 unsigned int *size)
{
    z_chunkq_node_t *node;
    unsigned int n = 0U;

    node = chunkq->head;
    while (node != NULL) {
        n += node->size;
        if (n > offset)
            break;

        node = node->next;
    }

    *size = n;
    return(node);
}

static long __chunkq_search_step (z_chunkq_search_t *search,
                                  const uint8_t *needle,
                                  unsigned int needle_len)
{
    long index = -1;
    void *p;

    while ((p = z_memchr(search->data, *(search->needle), search->data_len))) {
        if ((index = ((uint8_t *)p - search->data) + 1) > 1 && search->state) {
            search->needle_len = needle_len;
            search->needle = needle;
            search->state = 0;
            continue;
        }

        search->rd += index;
        if (!search->state) {
            search->offset = search->rd - 1;
            search->state = 1;
        }

        if (--(search->needle_len) == 0)
            return(1);

        search->needle++;
        search->data_len -= index;
        search->data += index;

        while (search->data_len > 0) {
            search->rd++;

            if (*(search->data) != *(search->needle)) {
                search->needle_len = needle_len;
                search->needle = needle;
                search->state = 0;
                break;
            }

            if (--(search->needle_len) == 0)
                return(2);

            search->needle++;
            search->data++;
            search->data_len--;
        }

        if (search->data_len == 0)
            return(0);
    }

    if (index < 0)
        search->rd += search->data_len;

    return(0);
}

z_chunkq_t *z_chunkq_alloc (z_chunkq_t *chunkq,
                            z_memory_t *memory,
                            unsigned int chunk_size)
{
    if ((chunkq = z_object_alloc(memory, chunkq, z_chunkq_t)) != NULL) {
        chunkq->chunk = z_align_up(chunk_size, 64);
        chunkq->head = NULL;
        chunkq->tail = NULL;
        chunkq->size = 0U;
    }

    return(chunkq);
}

void z_chunkq_free (z_chunkq_t *chunkq) {
    z_chunkq_clear(chunkq);
    z_object_free(chunkq);
}

void z_chunkq_clear (z_chunkq_t *chunkq) {
    z_chunkq_node_t *node;

    while ((node = chunkq->head) != NULL) {
        chunkq->head = node->next;
        __chunkq_node_free(chunkq, node);
    }

    chunkq->tail = NULL;
    chunkq->size = 0U;
}

unsigned int z_chunkq_pop (z_chunkq_t *chunkq,
                           void *buffer,
                           unsigned int size)
{
    uint8_t *p = (uint8_t *)buffer;
    z_chunkq_node_t *node;
    z_chunkq_node_t *next;
    unsigned int block;
    unsigned int n;
    int pop;

    if ((node = chunkq->head) == NULL)
        return(0);

    pop = 0;
    if ((block = node->size) > size) {
        block = size;
        pop = 1;
    }

    z_memcpy(p, node->data + node->offset, block);
    chunkq->size -= block;
    p += block;
    n = block;

    next = node->next;
    if (block == node->size) {
        if (node == chunkq->head)
            chunkq->head = next;

        __chunkq_node_free(chunkq, node);
    } else {
        node->offset += block;
        node->size -= block;
        return(n);
    }

    while (n < size && next != NULL) {
        node = next;

        pop = 0;
        block = (size - n);
        if (node->size <= block) {
            block = node->size;
            pop = 1;
        }

        z_memcpy(p, node->data + node->offset, block);
        chunkq->size -= block;
        p += block;
        n += block;

        next = node->next;
        if (pop) {
            if (node == chunkq->head)
                chunkq->head = next;

            __chunkq_node_free(chunkq, node);
        } else {
            node->offset += block;
            node->size -= block;
            break;
        }
    }

    if (chunkq->head == NULL)
        chunkq->tail = NULL;

    return(n);
}

unsigned int z_chunkq_read (z_chunkq_t *chunkq,
                            unsigned int offset,
                            void *buffer,
                            unsigned int size)
{
    uint8_t *p = (uint8_t *)buffer;
    z_chunkq_node_t *node;
    unsigned int block;
    unsigned int n;

    if ((node = __chunkq_node_at_offset(chunkq, offset, &n)) == NULL)
        return(0);

    offset = offset - (n - node->size);
    if ((block = (node->size - offset)) > size)
        block = size;

    z_memcpy(p, node->data + node->offset + offset, block);
    p += block;
    n = block;

    while (n < size) {
        if ((node = node->next) == NULL)
            break;

        block = (size - n);
        if (node->size < block)
            block = node->size;

        z_memcpy(p, node->data + node->offset, block);
        p += block;
        n += block;
    }

    return(n);
}

unsigned int z_chunkq_fetch (z_chunkq_t *chunkq,
                             unsigned int offset,
                             unsigned int size,
                             z_iopush_t fetch_func,
                             void *user_data)
{
    z_chunkq_node_t *node;
    unsigned int block;
    unsigned int n;

    if ((node = __chunkq_node_at_offset(chunkq, offset, &n)) == NULL)
        return(0);

    offset = offset - (n - node->size);
    if ((block = (node->size - offset)) > size)
        block = size;

    fetch_func(user_data, node->data + node->offset + offset, block);

    n = block;
    while (n < size) {
        if ((node = node->next) == NULL)
            break;

        block = (size - n);
        if (node->size < block)
            block = node->size;

        fetch_func(user_data, node->data + node->offset, block);
        n += block;
    }

    return(n);
}

static unsigned int __fetch_slice (void *data, void *buffer, unsigned int n) {
    z_chunkq_slice_info_t *info = (z_chunkq_slice_info_t *)data;
    unsigned int rd;

    if (info->available < n)
        n = info->available;

    rd = z_slice_copy(info->slice, info->offset, buffer, n);
    info->available -= rd;
    info->offset += rd;

    return(rd);
}

unsigned int z_chunkq_append_slice (z_chunkq_t *chunkq,
                                     const z_slice_t *slice)
{
    z_chunkq_slice_info_t info;

    info.available = z_slice_length(slice);
    info.offset = 0U;
    info.slice = slice;

    return(z_chunkq_append_fetch(chunkq, __fetch_slice, &info));
}

unsigned int z_chunkq_update_fetch (z_chunkq_t *chunkq,
                                    unsigned int offset,
                                    z_iofetch_t fetch_func,
                                    void *user_data)
{
    z_chunkq_node_t *node;
    unsigned int n, rd;

    if ((node = __chunkq_node_at_offset(chunkq, offset, &n)) == NULL)
        return(-1);

    if (node == NULL)
        return(z_chunkq_append_fetch(chunkq, fetch_func, user_data));

    /* Update chunkq */
    offset = offset - (n - node->size);
    n = (node->size - offset);

    rd = fetch_func(user_data, node->data + node->offset + offset, n);
    if (rd != n)
        return(rd);

    n = rd;
    while ((node = node->next) != NULL) {
        rd = fetch_func(user_data, node->data + node->offset, node->size);
        n += rd;

        if (rd != node->size)
            return(n);
    }

    /* TODO: Append to chunkq */

    return(0);
}

unsigned int z_chunkq_update_slice (z_chunkq_t *chunkq,
                                    unsigned int offset,
                                    const z_slice_t *slice)
{
    z_chunkq_slice_info_t info;

    info.available = z_slice_length(slice);
    info.offset = 0U;
    info.slice = slice;

    return(z_chunkq_update_fetch(chunkq, offset, __fetch_slice, &info));
}

unsigned int z_chunkq_append (z_chunkq_t *chunkq,
                              const void *buffer,
                              unsigned int size)
{
    const uint8_t *p = (const uint8_t *)buffer;
    z_chunkq_node_t *node;
    unsigned int block;
    unsigned int wr;

    if ((node = chunkq->tail) == NULL) {
        if ((node = __chunkq_node_alloc(chunkq)) == NULL)
            return(0);

        chunkq->tail = node;
        chunkq->head = node;
    }

    wr = 0U;
    while (wr < size) {
        if ((block = (chunkq->chunk - (node->offset + node->size))) == 0) {
            if ((node = __chunkq_node_alloc(chunkq)) == NULL)
                return(wr);

            chunkq->tail->next = node;
            chunkq->tail = node;
            block = chunkq->chunk;
        }

        if ((size - wr) < block)
            block = (size - wr);

        z_memcpy(node->data + node->offset + node->size, p, block);
        node->size += block;
        chunkq->size += block;
        wr += block;
        p += block;
    }

    return(wr);
}

unsigned int z_chunkq_append_fetch (z_chunkq_t *chunkq,
                                    z_iofetch_t fetch_func,
                                    void *user_data)
{
    z_chunkq_node_t *node;
    unsigned int readed;
    unsigned int n;

    if ((node = chunkq->tail) == NULL) {
        if ((node = __chunkq_node_alloc(chunkq)) == NULL)
            return(0);

        chunkq->tail = node;
        chunkq->head = node;
    }

    if ((n = (chunkq->chunk - (node->offset + node->size))) == 0) {
        if ((node = __chunkq_node_alloc(chunkq)) == NULL)
            return(0);

        chunkq->tail->next = node;
        chunkq->tail = node;
        n = chunkq->chunk;
    }

    if (!(n = fetch_func(user_data, node->data + node->offset + node->size, n)))
        return(0);

    node->size += n;
    chunkq->size += n;

    /* You've not read the whole block... */
    if ((node->offset + node->size) != chunkq->chunk)
        return(n);

    readed = n;
    do {
        /* Alloc new block */
        if ((node = __chunkq_node_alloc(chunkq)) == NULL)
            break;

        chunkq->tail->next = node;
        chunkq->tail = node;

        if (!(n = fetch_func(user_data, node->data, chunkq->chunk)))
            break;

        readed += n;
        node->size = n;
        chunkq->size += n;
    } while (n == chunkq->chunk);

    return(readed);
}

static unsigned int __fetch_fd (void *fd, void *buffer, unsigned int n) {
    ssize_t rd;
    if ((rd = read(Z_INT_PTR_VALUE(fd), buffer, n)) <= 0)
        return(0);

#if 0
    ssize_t i;
    for (i = 0; i < rd; ++i)
        putchar(((const char *)buffer)[i]);
#endif

    return(rd);
}

unsigned int z_chunkq_append_fetch_fd (z_chunkq_t *chunkq, int fd) {
    return(z_chunkq_append_fetch(chunkq, __fetch_fd, &fd));
}

unsigned int z_chunkq_remove_push (z_chunkq_t *chunkq,
                                   z_iopush_t push_func,
                                   void *user_data)
{
    z_chunkq_node_t *node;
    unsigned int pushed;
    unsigned int n;

    pushed = 0;
    while ((node = chunkq->head) != NULL) {
        n = push_func(user_data, node->data + node->offset, node->size);
        if (n != node->size) {
            node->offset += n;
            node->size -= n;
            break;
        }

        chunkq->size -= node->size;

#if 1
        if (node->next == NULL) {
            node->offset = 0;
            node->size = 0;
            break;
        }
#endif
        chunkq->head = node->next;
        __chunkq_node_free(chunkq, node);
    }

#if 0
    if (chunkq->head == NULL)
        chunkq->tail = NULL;
#endif

    return(pushed);
}

static unsigned int __push_fd (void *fd, const void *buffer, unsigned int n) {
    ssize_t wr;

    if ((wr = write(Z_INT_PTR_VALUE(fd), buffer, n)) <= 0)
        return(0);

#if 0
    ssize_t i;
    for (i = 0; i < wr; ++i)
        putchar(((const char *)buffer)[i]);
#endif
    return(wr);
}

unsigned int z_chunkq_remove_push_fd (z_chunkq_t *chunkq, int fd) {
    return(z_chunkq_remove_push(chunkq, __push_fd, &fd));
}

unsigned int z_chunkq_prepend (z_chunkq_t *chunkq,
                               const void *buffer,
                               unsigned int size)
{
    const uint8_t *p = (const uint8_t *)buffer;
    z_chunkq_node_t *node;
    unsigned int remains;
    unsigned int block;
    unsigned int wr;

    if ((node = chunkq->tail) == NULL) {
        if ((node = __chunkq_node_alloc(chunkq)) == NULL)
            return(0);

        chunkq->tail = node;
        chunkq->head = node;
    }

    wr = 0U;
    remains = size;
    while (wr < size) {
        if (node->offset == 0 && node->size > 0) {
            if ((node = __chunkq_node_alloc(chunkq)) == NULL)
                return(wr);

            node->next = chunkq->head;
            chunkq->head = node;

            block = chunkq->chunk;
            if (remains < block) {
                node->offset = block - remains;
                block = remains;
            }
        } else {
            if (node->size == 0) {
                block = chunkq->chunk;
                if (remains < block) {
                    node->offset = block - remains;
                    block = remains;
                }
            } else {
                block = (remains < node->offset) ? remains : node->offset;
                node->offset -= remains;
            }
        }

        z_memcpy(node->data + node->offset, p + remains - block, block);
        node->size += block;
        chunkq->size += block;
        remains += block;
        wr += block;
        p += block;
    }

    return(wr);
}

void z_chunkq_remove (z_chunkq_t *chunkq, unsigned int size) {
    z_chunkq_node_t *node;
    z_chunkq_node_t *next;
    unsigned int block;
    unsigned int n;
    int pop;

    if ((node = chunkq->head) == NULL)
        return;

    pop = 0;
    if ((block = node->size) > size) {
        block = size;
        pop = 1;
    }

    chunkq->size -= block;
    n = block;

    next = node->next;
    if (block == node->size) {
        if (node == chunkq->head)
            chunkq->head = next;

        __chunkq_node_free(chunkq, node);
    } else {
        node->offset += block;
        node->size -= block;
        return;
    }

    while (n < size && next != NULL) {
        node = next;

        pop = 0;
        block = (size - n);
        if (node->size <= block) {
            block = node->size;
            pop = 1;
        }

        chunkq->size -= block;
        n += block;

        next = node->next;
        if (pop) {
            if (node == chunkq->head)
                chunkq->head = next;

            __chunkq_node_free(chunkq, node);
        } else {
            node->offset += block;
            node->size -= block;
            break;
        }
    }

    if (chunkq->head == NULL)
        chunkq->tail = NULL;
}

long z_chunkq_indexof (z_chunkq_t *chunkq,
                       unsigned int offset,
                       const void *needle,
                       unsigned int needle_len)
{
    z_chunkq_search_t search;
    z_chunkq_node_t *node;
    unsigned int n;

    if ((node = __chunkq_node_at_offset(chunkq, offset, &n)) == NULL)
        return(-1);

    search.state = 0;
    search.offset = 0;
    search.rd = offset;
    search.needle = (const uint8_t *)needle;
    search.needle_len = needle_len;

    offset = offset - (n - node->size);
    search.data = (node->data + node->offset + offset);
    search.data_len = (node->size - offset);
    if (__chunkq_search_step(&search, (const uint8_t *)needle, needle_len))
        return(search.offset);

    while ((node = node->next) != NULL) {
        search.data = (node->data + node->offset);
        search.data_len = node->size;
        if (__chunkq_search_step(&search, (const uint8_t *)needle, needle_len))
            return(search.offset);
    }

    return(-1);
}

long z_chunkq_startswith (z_chunkq_t *chunkq,
                          unsigned int offset,
                          const void *needle,
                          unsigned int needle_len)
{
    return(z_chunkq_indexof(chunkq, offset, needle, needle_len) == offset);
}

int z_chunkq_compare (z_chunkq_t *chunkq,
                      unsigned int offset,
                      const void *mem,
                      unsigned int mem_size,
                      z_chunkq_compare_t cmp_func)
{
    z_chunkq_node_t *node;
    const uint8_t *a;
    const uint8_t *b;
    unsigned int n;
    int res;

    if ((node = __chunkq_node_at_offset(chunkq, offset, &n)) == NULL)
        return(-1);

    offset = offset - (n - node->size);
    n = node->size - offset;
    if (n >= mem_size)
        return(cmp_func(node->data + node->offset + offset, mem, mem_size));

    a = node->data + node->offset + offset;
    b = (const uint8_t *)mem;
    while (!(res = cmp_func(a, b, n))) {
        if ((mem_size = mem_size - n) == 0)
            break;

        b += n;
        node = node->next;
        a = node->data + node->offset;
        n = (node->size > mem_size) ? mem_size : node->size;
    }

    return(res);
}

int z_chunkq_memcmp (z_chunkq_t *chunkq,
                     unsigned int offset,
                     const void *mem,
                     unsigned int mem_size)
{
    return(z_chunkq_compare(chunkq, offset, mem, mem_size, z_memcmp));
}

int z_chunkq_tokenize (z_chunkq_t *chunkq,
                       z_chunkq_extent_t *extent,
                       unsigned int offset,
                       const char *tokens,
                       unsigned int ntokens)
{
    z_chunkq_node_t *node;
    const uint8_t *p;
    unsigned int n;

    if ((node = __chunkq_node_at_offset(chunkq, offset, &n)) == NULL)
        return(0);

    extent->chunkq = chunkq;
    extent->offset = offset;

    /* Skip until *p doesn't contains skip_chrs */
    offset = (offset - (n - node->size));
    n = node->size - offset;
    p = node->data + node->offset + offset;
    while (n--) {
        if (z_memchr(tokens, *p, ntokens) == NULL)
            goto _get_tok_end;

        extent->offset++;
        p++;
    }

    for (node = node->next; node != NULL; node = node->next) {
        n = node->size;
        p = node->data + node->offset;

        while (n--) {
            if (z_memchr(tokens, *p, ntokens) == NULL)
                goto _get_tok_end;

            extent->offset++;
            p++;
        }
    }

    return(0);

_get_tok_end:
    n++;
    extent->length = 0;
    while (node != NULL) {
        while (n--) {
            if (z_memchr(tokens, *p, ntokens) != NULL)
                goto _tok_length;

            extent->length++;
            p++;
        }

        if ((node = node->next) == NULL)
            break;

        p = node->data + node->offset;
        n = node->size;
    }

_tok_length:
    return(extent->length);
}

int z_chunkq_i32 (z_chunkq_t *chunkq,
                  unsigned int offset,
                  unsigned int length,
                  int base,
                  int32_t *value)
{
    char buffer[32];

    if (length > sizeof(buffer)) length = sizeof(buffer) - 1;
    z_chunkq_read(chunkq, offset, buffer, length);
    buffer[length] = '\0';

    return(z_strtoi32(buffer, base, value));
}

int z_chunkq_i64 (z_chunkq_t *chunkq,
                  unsigned int offset,
                  unsigned int length,
                  int base,
                  int64_t *value)
{
    char buffer[32];

    if (length > sizeof(buffer)) length = sizeof(buffer) - 1;
    z_chunkq_read(chunkq, offset, buffer, length);
    buffer[length] = '\0';

    return(z_strtoi64(buffer, base, value));
}

int z_chunkq_u32 (z_chunkq_t *chunkq,
                  unsigned int offset,
                  unsigned int length,
                  int base,
                  uint32_t *value)
{
    char buffer[32];

    if (length > sizeof(buffer)) length = sizeof(buffer) - 1;
    z_chunkq_read(chunkq, offset, buffer, length);
    buffer[length] = '\0';

    return(z_strtou32(buffer, base, value));
}

int z_chunkq_u64 (z_chunkq_t *chunkq,
                  unsigned int offset,
                  unsigned int length,
                  int base,
                  uint64_t *value)
{
    char buffer[32];

    if (length > sizeof(buffer)) length = sizeof(buffer) - 1;
    z_chunkq_read(chunkq, offset, buffer, length);
    buffer[length] = '\0';

    return(z_strtou64(buffer, base, value));
}

int z_chunkq_string (z_chunkq_t *chunkq,
                     z_string_t *string,
                     unsigned int offset,
                     unsigned int length)
{
    char *blob;

    if (length == 0)
        return(1);

    if ((blob = z_memory_alloc(z_object_memory(chunkq), length)) == NULL)
        return(2);

    z_chunkq_read(chunkq, offset, blob, length);
    z_string_set(string, blob, length);

    return(0);
}

void z_chunkq_dump (const z_chunkq_extent_t *extent) {
    unsigned int n, rd;
    unsigned int offset;
    char buffer[4096];
    unsigned int i;

    printf("z_chunkq_dump(): %p:%u:%u - ", extent->chunkq, extent->offset, extent->length);

    n = extent->length;
    offset = extent->offset;
    while (n > 0) {
        rd = (n > sizeof(buffer)) ? sizeof(buffer) : n;
        rd = z_chunkq_read(extent->chunkq, offset, buffer, rd);
        n -= rd;
        offset += rd;

        for (i = 0; i < rd; ++i)
            printf("%c", buffer[i]);
    }

    printf("\n");
}

static unsigned int __stream_push (void *user_data,
                                   const void *buffer,
                                   unsigned int n)
{
    z_stream_write(Z_STREAM(user_data), buffer, n);
    return(n);
}

int z_stream_write_chunkq (z_stream_t *stream,
                           z_chunkq_t *chunkq,
                           unsigned int offset,
                           unsigned int length)
{
    z_chunkq_fetch(chunkq, offset, length, __stream_push, stream);
    return(0);
}

/* ============================================================================
 *  Chunkq Stream
 */
static uint64_t __chunkq_tell (z_stream_t *stream) {
    return(((z_chunkq_stream_t *)stream)->offset);
}

static int __chunkq_seek (z_stream_t *stream, uint64_t offset) {
    z_chunkq_stream_t *cstream = (z_chunkq_stream_t *)stream;
    cstream->offset = (unsigned int)(offset & 0xffffffff);
    return(0);
}

static int __chunkq_read (z_stream_t *stream,
                          void *buffer,
                          unsigned int n)
{
    z_chunkq_stream_t *cstream = (z_chunkq_stream_t *)stream;
    unsigned int rd;

    rd = z_chunkq_read(cstream->chunkq, cstream->offset, buffer, n);
    cstream->offset += rd;
    return(rd);
}

static int __chunkq_write (z_stream_t *stream,
                           const void *buffer,
                           unsigned int n)
{
    z_chunkq_stream_t *cstream = (z_chunkq_stream_t *)stream;
    unsigned int wr;

    wr = z_chunkq_append(cstream->chunkq, buffer, n);
    cstream->offset += wr;
    return(wr);
}

#if 0
static int __chunkq_fetch (z_stream_t *stream,
                           unsigned int length,
                           z_iopush_t fetch_func,
                           void *user_data)
{
    z_chunkq_stream_t *cstream = (z_chunkq_stream_t *)stream;
    unsigned int rd;

    rd = z_chunkq_fetch(cstream->chunkq, cstream->offset, length,
                        fetch_func, user_data);
    cstream->offset += rd;

    return(rd);
}
#endif

static z_stream_vtable_t __chunkq_stream_vtable = {
    .tell   = __chunkq_tell,
    .seek   = __chunkq_seek,
    .read   = __chunkq_read,
    .write  = __chunkq_write,
    /* .fetch  = __chunkq_fetch, */
};

int z_chunkq_stream (z_chunkq_stream_t *stream,
                     z_chunkq_t *chunkq)
{
    stream->__base_type__.vtable = &__chunkq_stream_vtable;
    stream->chunkq = chunkq;
    stream->offset = 0;
    return(0);
}

