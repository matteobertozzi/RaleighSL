#include <zcl/bitops.h>

#define __FFS_STEP(r, x, y, z)     if (!((x) & (z))) { x >>= y; r += y; }

unsigned int z_ffs8 (uint8_t x) {
    unsigned int r = 1;

    if (!x) return(0);
    __FFS_STEP(r, x,  4, 0xf)
    __FFS_STEP(r, x,  2, 0x3)
    __FFS_STEP(r, x,  1, 0x1)
    return(r);
}

unsigned int z_ffs16 (uint16_t x) {
    unsigned int r = 1;

    if (!x) return(0);
    __FFS_STEP(r, x,  8, 0xff)
    __FFS_STEP(r, x,  4, 0xf)
    __FFS_STEP(r, x,  2, 0x3)
    __FFS_STEP(r, x,  1, 0x1)
    return(r);
}

unsigned int z_ffs32 (uint32_t x) {
    unsigned int r = 1;

    if (!x) return(0);
    __FFS_STEP(r, x, 16, 0xffff)
    __FFS_STEP(r, x,  8, 0xff)
    __FFS_STEP(r, x,  4, 0xf)
    __FFS_STEP(r, x,  2, 0x3)
    __FFS_STEP(r, x,  1, 0x1)
    return(r);
}

unsigned int z_ffs64 (uint64_t x) {
    unsigned int r = 1;

    if (!x) return(0);
    __FFS_STEP(r, x, 32, 0xffffffff)
    __FFS_STEP(r, x, 16, 0xffff)
    __FFS_STEP(r, x,  8, 0xff)
    __FFS_STEP(r, x,  4, 0xf)
    __FFS_STEP(r, x,  2, 0x3)
    __FFS_STEP(r, x,  1, 0x1)
    return(r);
}

static unsigned int __ff_bit (const void *map,
                              unsigned int offset,
                              unsigned int size,
                              int value)
{
#if __LP64__
    const uint64_t pattern64[2] = { 0xffffffffffffffff, 0x0000000000000000 };
    const uint64_t *u64;
#else
    const uint32_t pattern32[2] = { 0xffffffff, 0x00000000 };
    const uint32_t *u32;
#endif
	const uint8_t pattern[2] = { 0xff, 0x00 };
	const uint8_t *p;
	int bit;

    p = (const uint8_t *)map + (offset >> 3);
    size = size - offset;
    if ((bit = offset & 0x7)) {
        for (; bit < 8; ++bit) {
            if (z_bit_test(p, bit) == value)
                return(1 + ((p - (const uint8_t *)map) << 3) + bit);
        }

        if (--size == 0)
            return(0);
        p++;
    }

#if __LP64__
    u64 = (const uint64_t *)p;
    while (size >= 8 && *u64 == pattern64[value]) {
        size -= 8;
        u64++;
    }
    p = (const uint8_t *)u64;
#else
    u32 = (const uint32_t *)p;
    while (size >= 4 && *u32 == pattern32[value]) {
        size -= 4;
        u32++;
    }
    p = (const uint8_t *)u32;
#endif

    if (size > 0) {
    	while (*p++ == pattern[value]) {
    		if (--size == 0)
    			return(0);
    	}
        --p;

        bit = z_ffs8(value ? *p : ~(*p));
        return(((p - (const uint8_t *)map) << 3) + bit);
    }

    return(0);
}

static unsigned int __ff_area (const void *map,
                               unsigned int offset,
                               unsigned int size,
                               unsigned int nr,
                               int value)
{
    unsigned int s, e, x;

    while (offset < size) {
        if (!(s = __ff_bit(map, offset, size, value)))
            return(0);

        if ((e = s + nr - 1) > size)
            return(0);

        x = __ff_bit(map, s, e, !value);
        if (!x || x > e)
            return(s);

        offset = x;
    }

    return(0);
}

unsigned int z_ffs (const void *map, unsigned int offset, unsigned int size) {
    return(__ff_bit(map, offset, size, 1));
}

unsigned int z_ffz (const void *map, unsigned int offset, unsigned int size) {
    return(__ff_bit(map, offset, size, 0));
}

unsigned int z_ffs_area (const void *map,
                         unsigned int offset,
                         unsigned int size,
                         unsigned int nr)
{
    return(__ff_area(map, offset, size, nr, 1));
}

unsigned int z_ffz_area (const void *map,
                         unsigned int offset,
                         unsigned int size,
                         unsigned int nr)
{
    return(__ff_area(map, offset, size, nr, 0));
}

