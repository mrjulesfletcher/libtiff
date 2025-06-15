#include "tif_bayer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int test_depth(int bits)
{
    const size_t count = 32;
    uint16_t *src = (uint16_t *)malloc(count * sizeof(uint16_t));
    uint16_t *dst = (uint16_t *)malloc(count * sizeof(uint16_t));
    if (!src || !dst)
        return 1;
    for (size_t i = 0; i < count; i++)
        src[i] = (uint16_t)(i & ((1U << bits) - 1));
    size_t bytes;
    switch (bits)
    {
    case 10:
        bytes = ((count + 3) / 4) * 5;
        break;
    case 12:
        bytes = ((count + 1) / 2) * 3;
        break;
    case 14:
        bytes = ((count + 3) / 4) * 7;
        break;
    default:
        bytes = count * 2;
        break;
    }
    uint8_t *buf = (uint8_t *)malloc(bytes);
    if (!buf)
        return 1;
    switch (bits)
    {
    case 10:
        TIFFPackRaw10(src, buf, count, 0);
        TIFFUnpackRaw10(buf, dst, count, 0);
        break;
    case 12:
        TIFFPackRaw12(src, buf, count, 0);
        TIFFUnpackRaw12(buf, dst, count, 0);
        break;
    case 14:
        TIFFPackRaw14(src, buf, count, 0);
        TIFFUnpackRaw14(buf, dst, count, 0);
        break;
    case 16:
        TIFFPackRaw16(src, buf, count, 0);
        TIFFUnpackRaw16(buf, dst, count, 0);
        break;
    }
    int ret = memcmp(src, dst, count * sizeof(uint16_t)) != 0;
    free(src);
    free(dst);
    free(buf);
    return ret;
}

int main(void)
{
    if (test_depth(10)) return 1;
    if (test_depth(12)) return 1;
    if (test_depth(14)) return 1;
    if (test_depth(16)) return 1;
    return 0;
}
