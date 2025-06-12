#include "tiffio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    const uint32_t w = 32;
    const uint32_t h = 8;
    uint8_t *buf = (uint8_t *)malloc(w * h);
    if (!buf)
        return 1;
    for (uint32_t i = 0; i < w * h; i++)
        buf[i] = (uint8_t)(i & 0xff);

    TIFF *tif = TIFFOpen("gray_flip_neon.tif", "w");
    if (!tif)
    {
        free(buf);
        return 1;
    }
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, w);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, h);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_BOTRIGHT);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, h);
    if (TIFFWriteEncodedStrip(tif, 0, buf, w * h) == -1)
    {
        TIFFClose(tif);
        free(buf);
        return 1;
    }
    TIFFClose(tif);

    tif = TIFFOpen("gray_flip_neon.tif", "r");
    if (!tif)
    {
        free(buf);
        return 1;
    }
    uint32_t *raster = (uint32_t *)malloc(w * h * sizeof(uint32_t));
    if (!raster)
    {
        TIFFClose(tif);
        free(buf);
        return 1;
    }
    if (!TIFFReadRGBAImage(tif, w, h, raster, 0))
    {
        TIFFClose(tif);
        free(buf);
        free(raster);
        return 1;
    }
    TIFFClose(tif);

    int ret = 0;
    for (uint32_t y = 0; y < h && !ret; y++)
    {
        for (uint32_t x = 0; x < w; x++)
        {
            uint8_t g = buf[y * w + (w - 1 - x)];
            uint32_t exp = 0xff000000U | ((uint32_t)g) | ((uint32_t)g << 8) |
                            ((uint32_t)g << 16);
            if (raster[y * w + x] != exp)
            {
                ret = 1;
                break;
            }
        }
    }
    free(buf);
    free(raster);
    remove("gray_flip_neon.tif");
    return ret;
}
