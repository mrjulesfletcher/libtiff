#include "tiffio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    TIFFInitSIMD();
    const uint32_t w = 32;
    const uint32_t h = 8;
    size_t n = (size_t)w * h;
    unsigned char *buf = (unsigned char *)malloc(n * 3);
    if (!buf)
        return 1;
    for (size_t i = 0; i < n; i++)
    {
        buf[i * 3 + 0] = (unsigned char)(i & 0xFF);
        buf[i * 3 + 1] = (unsigned char)(128 + ((i * 2) & 0x7F));
        buf[i * 3 + 2] = (unsigned char)(128 + ((i * 3) & 0x7F));
    }

    TIFF *tif = TIFFOpen("ycbcr_neon.tif", "w");
    if (!tif)
    {
        free(buf);
        return 1;
    }
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, w);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, h);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, h);
    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_YCBCR);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    uint16_t subs_h = 1, subs_v = 1;
    TIFFSetField(tif, TIFFTAG_YCBCRSUBSAMPLING, subs_h, subs_v);
    if (TIFFWriteEncodedStrip(tif, 0, buf, n * 3) == -1)
    {
        TIFFClose(tif);
        free(buf);
        return 1;
    }
    TIFFClose(tif);

    tif = TIFFOpen("ycbcr_neon.tif", "r");
    if (!tif)
    {
        free(buf);
        return 1;
    }

    uint32_t *ref = (uint32_t *)malloc(n * sizeof(uint32_t));
    uint32_t *simd = (uint32_t *)malloc(n * sizeof(uint32_t));
    if (!ref || !simd)
    {
        TIFFClose(tif);
        free(buf);
        free(ref);
        free(simd);
        return 1;
    }

    int neon = TIFFUseNEON();
    TIFFSetUseNEON(0);
    if (!TIFFReadRGBAImage(tif, w, h, ref, 0))
    {
        TIFFClose(tif);
        free(buf);
        free(ref);
        free(simd);
        return 1;
    }
    TIFFSetUseNEON(neon);
    if (!TIFFReadRGBAImage(tif, w, h, simd, 0))
    {
        TIFFClose(tif);
        free(buf);
        free(ref);
        free(simd);
        return 1;
    }
    TIFFClose(tif);

    int ret = memcmp(ref, simd, n * sizeof(uint32_t)) != 0;
    free(buf);
    free(ref);
    free(simd);
    remove("ycbcr_neon.tif");
    return ret;
}
