#include "strip_neon.h"
#include "tiffio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static double elapsed_ms(struct timespec *s, struct timespec *e)
{
    return (e->tv_sec - s->tv_sec) * 1000.0 +
           (e->tv_nsec - s->tv_nsec) / 1000000.0;
}

int main(void)
{
    const char *filename = "uring_pack_bench.tiff";
    const uint32_t width = 512, height = 512;
    size_t count = (size_t)width * height;
    uint16_t *src = (uint16_t *)malloc(count * sizeof(uint16_t));
    if (!src)
        return 1;
    for (size_t i = 0; i < count; i++)
        src[i] = (uint16_t)(i & 0x0FFF);

    size_t strip_size = 0;
    uint8_t *packed =
        TIFFAssembleStripNEON(NULL, src, width, height, 1, 0, &strip_size);

    struct timespec s, e;
    clock_gettime(CLOCK_MONOTONIC, &s);
    TIFF *tif = TIFFOpen(filename, "w");
    if (!tif)
    {
        fprintf(stderr, "cannot create %s\n", filename);
        free(packed);
        free(src);
        return 1;
    }
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 12);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, height);
    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    if (TIFFWriteRawStrip(tif, 0, packed, strip_size) != (tmsize_t)strip_size)
    {
        fprintf(stderr, "write failed\n");
        TIFFClose(tif);
        free(packed);
        free(src);
        return 1;
    }
    TIFFClose(tif);
    clock_gettime(CLOCK_MONOTONIC, &e);
    printf("write: %.2f ms\n", elapsed_ms(&s, &e));

    uint8_t *readbuf = (uint8_t *)malloc(strip_size);
    clock_gettime(CLOCK_MONOTONIC, &s);
    tif = TIFFOpen(filename, "r");
    if (!tif)
    {
        fprintf(stderr, "cannot reopen %s\n", filename);
        free(readbuf);
        free(packed);
        free(src);
        return 1;
    }
    if (TIFFReadRawStrip(tif, 0, readbuf, strip_size) != (tmsize_t)strip_size)
    {
        fprintf(stderr, "read failed\n");
        TIFFClose(tif);
        free(readbuf);
        free(packed);
        free(src);
        return 1;
    }
    TIFFClose(tif);
    clock_gettime(CLOCK_MONOTONIC, &e);
    printf("read: %.2f ms\n", elapsed_ms(&s, &e));

    int ret = memcmp(readbuf, packed, strip_size) != 0;
    free(readbuf);
    free(packed);
    free(src);
    unlink(filename);
    return ret;
}
