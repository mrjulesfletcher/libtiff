#include "tif_config.h"
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "tiffio.h"
#include "tiffiop.h"

#define MY_EXIF_TAG 65000
#define MY_DNG_TAG 65001

static const TIFFFieldInfo customFieldInfo[] = {
    { MY_EXIF_TAG, -1, -1, TIFF_ASCII, FIELD_CUSTOM, 1, 0, "MyExifTag" },
    { MY_DNG_TAG, 1, 1, TIFF_LONG, FIELD_CUSTOM, 1, 0, "MyDngTag" }
};

static TIFFExtendProc parent_extender = NULL;

static void customTagExtender(TIFF* tif)
{
    fprintf(stderr, "customTagExtender called\n");
    int ret = TIFFMergeFieldInfo(tif, customFieldInfo,
                                 sizeof(customFieldInfo) / sizeof(customFieldInfo[0]));
    if (ret != 0)
        fprintf(stderr, "TIFFMergeFieldInfo failed\n");
    if (parent_extender)
        parent_extender(tif);
}

int main(void)
{
    static const char filename[] = "register_custom_tags.tif";
    const char exif_value[] = "custom";
    uint32_t dng_value = 123456789;

    parent_extender = TIFFSetTagExtender(customTagExtender);

    TIFF* tif = TIFFOpen(filename, "w+");
    if (!tif)
    {
        fprintf(stderr, "Cannot create %s\n", filename);
        return 1;
    }

    if (!TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, 1) ||
        !TIFFSetField(tif, TIFFTAG_IMAGELENGTH, 1) ||
        !TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3) ||
        !TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8) ||
        !TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 1) ||
        !TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG) ||
        !TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB))
    {
        TIFFClose(tif);
        fprintf(stderr, "Cannot set basic tags\n");
        return 1;
    }

    if (!TIFFSetField(tif, MY_DNG_TAG, dng_value))
    {
        fprintf(stderr, "Failed to set MY_DNG_TAG\n");
        TIFFClose(tif);
        return 1;
    }
    else
    {
        fprintf(stderr, "Set MY_DNG_TAG success\n");
    }

    if (!TIFFSetField(tif, MY_EXIF_TAG, exif_value))
    {
        fprintf(stderr, "Failed to set MY_EXIF_TAG\n");
        TIFFClose(tif);
        return 1;
    }
    else
    {
        fprintf(stderr, "Set MY_EXIF_TAG success\n");
    }

    uint8_t buf[3] = {0, 0, 0};
    if (TIFFWriteScanline(tif, buf, 0, 0) == -1)
    {
        fprintf(stderr, "WriteScanline failed\n");
        TIFFClose(tif);
        return 1;
    }
    if (!TIFFWriteDirectory(tif))
    {
        fprintf(stderr, "TIFFWriteDirectory failed\n");
        TIFFClose(tif);
        return 1;
    }
    fprintf(stderr, "Wrote main directory\n");
    TIFFClose(tif);
    fprintf(stderr, "Closed write file\n");

    /* reopen and verify */
    tif = TIFFOpen(filename, "r");
    if (!tif)
    {
        fprintf(stderr, "Cannot reopen %s\n", filename);
        return 1;
    }
    uint32_t dng_read = 0;
    if (!TIFFGetField(tif, MY_DNG_TAG, &dng_read) || dng_read != dng_value)
    {
        fprintf(stderr, "MY_DNG_TAG read mismatch\n");
        TIFFClose(tif);
        return 1;
    }
    char* exif_read = NULL;
    if (!TIFFGetField(tif, MY_EXIF_TAG, &exif_read) || strcmp(exif_read, exif_value) != 0)
    {
        fprintf(stderr, "MY_EXIF_TAG read mismatch\n");
        TIFFClose(tif);
        return 1;
    }
    TIFFClose(tif);
    fprintf(stderr, "Closed read file\n");
    TIFFSetTagExtender(parent_extender);
#ifdef HAVE_UNISTD_H
    unlink(filename);
#endif
    return 0;
}

