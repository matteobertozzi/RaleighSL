#include <zcl/string.h>
#include <zcl/reader.h>
#include <zcl/slice.h>

#include <stdio.h>

struct reader {
    const char *buffer[6];
    size_t offset;
    size_t index;
};

static size_t __reader_next (void *self, uint8_t **data) {
    struct reader *reader = (struct reader *)self;
    size_t n;

    if (reader->index > 5)
        return(0);

    n = z_strlen(reader->buffer[reader->index]);
    if (reader->offset < n) {
        *data = ((uint8_t *)reader->buffer[reader->index]) + reader->offset;
        n -= reader->offset;
        reader->offset += n;
        return(n);
    }

    if (++reader->index > 5)
        return(0);

    reader->offset = z_strlen(reader->buffer[reader->index]);
    *data = (uint8_t *)reader->buffer[reader->index];
    return(reader->offset);
}

static void __reader_backup (void *self, size_t count) {
    struct reader *reader = (struct reader *)self;
    reader->offset -= count;
}

static const z_vtable_reader_t __reader_vtable = {
    .next       = __reader_next,
    .backup     = __reader_backup,
    .available  = NULL,
};

int main (int argc, char **argv) {
    struct reader reader;
    z_memory_t memory;
    z_slice_t slice;
    size_t nread;
    reader.buffer[0] = "Hey! this is a\n";
    reader.buffer[1] = "wrapping, text that@does!";
    reader.buffer[2] = "fancy";
    reader.buffer[3] = "-things";
    reader.buffer[4] = "-cool ah\n";
    reader.buffer[5] = "yoo.com...";
    reader.offset = 0;
    reader.index = 0;

    z_system_memory_init(&memory);
    z_slice_alloc(&slice, &memory);

    while ((nread = z_v_reader_tokenize(&__reader_vtable, &reader, " !\r\n@,.", 7, &slice)) > 0) {
        printf("nread=%lu slice=", nread); z_slice_dump(&slice);
    }

    z_slice_free(&slice);
    return(0);
}
