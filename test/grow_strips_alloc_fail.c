#include "failalloc.h"
#include "tiffio.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

int main(void)
{
    const char *fname = "grow_strips_alloc_fail.tif";
    setenv("FAIL_MALLOC_COUNT", "50", 1);
    failalloc_reset_from_env();
    TIFF *tif = TIFFOpen(fname, "w8");
    if (!tif)
    {
        fprintf(stderr, "Cannot create %s\n", fname);
        return 1;
    }
    int ret = 1;
    ret &= TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, 1);
    ret &= TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
    ret &= TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
    ret &= TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 1);
    ret &= TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    ret &= TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    if (!ret)
    {
        TIFFClose(tif);
        return 1;
    }
    uint8_t val = 0;
    for (uint32_t i = 0; i < 200; i++)
    {
        if (TIFFWriteScanline(tif, &val, i, 0) < 0)
        {
            TIFFClose(tif);
            return 1;
        }
    }
    TIFFClose(tif);
#ifdef HAVE_UNISTD_H
    unlink(fname);
#endif
    return 0;
}
