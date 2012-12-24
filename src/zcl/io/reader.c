#include <zcl/reader.h>
#include <zcl/extent.h>
#include <zcl/string.h>

int z_v_reader_search (const z_vtable_reader_t *vtable, void *self,
                       const uint8_t *needle, size_t needle_len,
                       z_extent_t *extent)
{
    uint8_t *buffer;
    size_t n;

    z_extent_reset(extent);
    while ((n = vtable->next(self, &buffer)) > 0) {
        size_t length = extent->length;
        if (!z_memsearch_step_u8(buffer, n, needle, needle_len, extent)) {
            vtable->backup(self, extent->length - length);
            return(0);
        }
    }

    return(1);
}