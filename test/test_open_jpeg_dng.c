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
    const char *jpeg_rel = "images/TEST_JPEG.jpg";
    const char *dng_rel = "images/TEST_CINEPI_LIBTIFF_DNG.dng";
    char *srcdir = getenv("srcdir");
    if (!srcdir)
        srcdir = ".";
    char path[1024];
    TIFF *tif;
    /* Try opening JPEG file. This is expected to fail since it is not a TIFF */
    snprintf(path, sizeof(path), "%s/%s", srcdir, jpeg_rel);
    tif = TIFFOpen(path, "r");
    if (tif)
    {
        fprintf(stderr, "Unexpectedly opened JPEG as TIFF: %s\n", path);
        TIFFClose(tif);
        return 1;
    }

    /* Open DNG file and read a few tags */
    snprintf(path, sizeof(path), "%s/%s", srcdir, dng_rel);
    tif = TIFFOpen(path, "r");
    if (!tif)
    {
        fprintf(stderr, "Cannot open %s\n", path);
        return 1;
    }
    uint32_t width = 0, length = 0;
    uint16_t bps = 0, spp = 0, photo = 0;
    if (!TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width) || width != 32)
    {
        fprintf(stderr, "Unexpected width %u\n", width);
        TIFFClose(tif);
        return 1;
    }
    if (!TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &length) || length != 32)
    {
        fprintf(stderr, "Unexpected length %u\n", length);
        TIFFClose(tif);
        return 1;
    }
    if (!TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps) || bps != 8)
    {
        fprintf(stderr, "Unexpected bps %u\n", bps);
        TIFFClose(tif);
        return 1;
    }
    if (!TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &spp) || spp != 1)
    {
        fprintf(stderr, "Unexpected spp %u\n", spp);
        TIFFClose(tif);
        return 1;
    }
    if (!TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photo) ||
        photo != PHOTOMETRIC_MINISBLACK)
    {
        fprintf(stderr, "Unexpected photometric %u\n", photo);
        TIFFClose(tif);
        return 1;
    }
    TIFFClose(tif);

    /* Create a small TIFF, write a white image, then verify */
    const char *newname = "test_roundtrip.tif";
    tif = TIFFOpen(newname, "w");
    if (!tif)
    {
        fprintf(stderr, "Cannot create %s\n", newname);
        return 1;
    }
    width = 10;
    length = 10;
    bps = 8;
    spp = 3;
    photo = PHOTOMETRIC_RGB;
    if (!TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width) ||
        !TIFFSetField(tif, TIFFTAG_IMAGELENGTH, length) ||
        !TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bps) ||
        !TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, spp) ||
        !TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, photo) ||
        !TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG))
    {
        fprintf(stderr, "Setting tags failed\n");
        TIFFClose(tif);
        return 1;
    }
    uint8_t row[30];
    memset(row, 255, sizeof(row));
    for (uint32_t y = 0; y < length; y++)
    {
        if (TIFFWriteScanline(tif, row, y, 0) == -1)
        {
            fprintf(stderr, "Write failed\n");
            TIFFClose(tif);
            return 1;
        }
    }
    TIFFClose(tif);

    tif = TIFFOpen(newname, "r");
    if (!tif)
    {
        fprintf(stderr, "Cannot reopen %s\n", newname);
        return 1;
    }
    uint32_t rwidth = 0, rlength = 0;
    uint16_t rbps = 0, rspp = 0, rphoto = 0;
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &rwidth);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &rlength);
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &rbps);
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &rspp);
    TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &rphoto);
    TIFFClose(tif);
    if (rwidth != width || rlength != length || rbps != bps || rspp != spp ||
        rphoto != photo)
    {
        fprintf(stderr, "Metadata mismatch after re-read\n");
        return 1;
    }
    unlink(newname);
    return 0;
}
