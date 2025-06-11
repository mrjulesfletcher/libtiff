#include "strip_neon.h"
#include "tiffio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    const uint32_t width = 32;
    const uint32_t height = 8;
    size_t count = width * height;
    uint16_t *buf = (uint16_t *)malloc(count * sizeof(uint16_t));
    if (!buf)
        return 1;
    for (size_t i = 0; i < count; i++)
        buf[i] = (uint16_t)i;

    size_t strip_size = 0;
    uint8_t *strip =
        TIFFAssembleStripNEON(NULL, buf, width, height, 0, 1, &strip_size);
    free(buf);
    if (!strip)
        return 1;

    TIFF *tif = TIFFOpen("assemble_strip_neon.tif", "w");
    if (!tif)
    {
        free(strip);
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

    if (TIFFWriteRawStrip(tif, 0, strip, strip_size) != (tmsize_t)strip_size)
    {
        TIFFClose(tif);
        free(strip);
        return 1;
    }
    TIFFClose(tif);

    tif = TIFFOpen("assemble_strip_neon.tif", "r");
    if (!tif)
    {
        free(strip);
        return 1;
    }
    uint8_t *read_buf = (uint8_t *)malloc(strip_size);
    if (!read_buf)
    {
        TIFFClose(tif);
        free(strip);
        return 1;
    }
    tmsize_t n = TIFFReadRawStrip(tif, 0, read_buf, strip_size);
    TIFFClose(tif);
    int ret = 0;
    if (n != (tmsize_t)strip_size || memcmp(read_buf, strip, strip_size) != 0)
        ret = 1;
    free(read_buf);
    free(strip);
    remove("assemble_strip_neon.tif");
    return ret;
}
