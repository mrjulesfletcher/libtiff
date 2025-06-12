#include "tif_config.h"
#include <stdio.h>
#include <unistd.h>
#include "tiffio.h"

int main(void)
{
    const char *filename = "uring_test.tiff";
    uint8_t buf[1] = {42};
    uint8_t rbuf[1] = {0};

    TIFF *tif = TIFFOpen(filename, "w");
    if (!tif)
    {
        fprintf(stderr, "cannot create %s\n", filename);
        return 1;
    }
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, 1);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, 1);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    if (TIFFWriteEncodedStrip(tif, 0, buf, 1) != 1)
    {
        fprintf(stderr, "write failed\n");
        TIFFClose(tif);
        return 1;
    }
    TIFFClose(tif);

    tif = TIFFOpen(filename, "r");
    if (!tif)
    {
        fprintf(stderr, "cannot reopen %s\n", filename);
        return 1;
    }
    if (TIFFReadEncodedStrip(tif, 0, rbuf, 1) != 1)
    {
        fprintf(stderr, "read failed\n");
        TIFFClose(tif);
        return 1;
    }
    TIFFClose(tif);
    unlink(filename);
    return (rbuf[0] == buf[0]) ? 0 : 1;
}
