#include "tiffio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    const char *filename = "packbits_literal_run.tif";
    const uint32_t width = 20000;
    const uint32_t height = 1;
    uint8_t *buf = (uint8_t *)malloc(width * height);
    if (!buf)
        return 1;
    for (uint32_t i = 0; i < width; i++)
        buf[i] = (uint8_t)(i & 0xFF);

    TIFF *tif = TIFFOpen(filename, "w");
    if (!tif)
    {
        free(buf);
        return 1;
    }
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_PACKBITS);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, height);
    if (TIFFWriteScanline(tif, buf, 0, 0) < 0)
    {
        TIFFClose(tif);
        free(buf);
        return 1;
    }
    TIFFClose(tif);

    tif = TIFFOpen(filename, "r");
    if (!tif)
    {
        free(buf);
        return 1;
    }
    uint8_t *read_buf = (uint8_t *)malloc(width);
    if (!read_buf)
    {
        TIFFClose(tif);
        free(buf);
        return 1;
    }
    if (TIFFReadScanline(tif, read_buf, 0, 0) < 0)
    {
        TIFFClose(tif);
        free(buf);
        free(read_buf);
        return 1;
    }
    TIFFClose(tif);

    int ret = memcmp(buf, read_buf, width) != 0;
    free(buf);
    free(read_buf);
    remove(filename);
    return ret;
}
