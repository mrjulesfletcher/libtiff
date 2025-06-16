#include "tif_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "tiffio.h"

int main(void)
{
#ifndef CHUNKY_STRIP_READ_SUPPORT
    printf("CHUNKY_STRIP_READ_SUPPORT not enabled. Skipping test.\n");
    return 0;
#else
    const char *srcdir = getenv("srcdir");
    if (!srcdir)
        srcdir = ".";
    char filename[1024];
    snprintf(filename, sizeof(filename), "%s/images/minisblack-1c-8b.tiff", srcdir);

    TIFF *tif = TIFFOpen(filename, "r");
    if (!tif)
    {
        fprintf(stderr, "Cannot open %s\n", filename);
        return 1;
    }

    uint32_t width = 0, height = 0;
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
    tmsize_t scanline = TIFFScanlineSize(tif);

    uint8_t *baseline = (uint8_t *)_TIFFmalloc(scanline * height);
    uint8_t *buf = (uint8_t *)_TIFFmalloc(scanline);
    if (!baseline || !buf)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    for (uint32_t row = 0; row < height; row++)
    {
        if (TIFFReadScanline(tif, baseline + row * scanline, row, 0) < 0)
        {
            fprintf(stderr, "TIFFReadScanline failed at row %u\n", row);
            return 1;
        }
    }
    TIFFClose(tif);

    tif = TIFFOpen(filename, "r");
    if (!tif)
    {
        fprintf(stderr, "Cannot reopen %s\n", filename);
        return 1;
    }

    uint32_t rows[] = {0, 10, 5, 20, 19, 2, height - 1, 1};
    int n = sizeof(rows) / sizeof(rows[0]);
    for (int i = 0; i < n; i++)
    {
        uint32_t r = rows[i];
        if (r >= height)
            r = height - 1;
        if (TIFFReadScanline(tif, buf, r, 0) < 0)
        {
            fprintf(stderr, "Seek/Read failed at row %u\n", r);
            return 1;
        }
        if (memcmp(buf, baseline + r * scanline, scanline) != 0)
        {
            fprintf(stderr, "Data mismatch at row %u\n", r);
            return 1;
        }
    }

    TIFFClose(tif);
    _TIFFfree(buf);
    _TIFFfree(baseline);
    fprintf(stderr, "test_seek_partial completed OK\n");
    return 0;
#endif
}
