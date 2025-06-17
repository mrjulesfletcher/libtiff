#include "tif_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "tiffio.h"

int main(int argc, char **argv)
{
    char path[1024];
    const char *filepath = NULL;
    if (argc >= 2)
    {
        filepath = argv[1];
    }
    else
    {
        filepath = getenv("BIGTIFF_FILE");
        if (!filepath)
        {
            const char *rel = "images/TEST_JPEG_BIG.tif";
            char *srcdir = getenv("srcdir");
            if (!srcdir)
                srcdir = ".";
            snprintf(path, sizeof(path), "%s/%s", srcdir, rel);
            filepath = path;
        }
    }

    int generated = 0;
    TIFF *in = TIFFOpen(filepath, "r");
    if (!in)
    {
        const char *srcdir = getenv("srcdir");
        if (!srcdir)
            srcdir = ".";
        char jpeg[1024];
        snprintf(jpeg, sizeof(jpeg), "%s/images/TEST_JPEG.jpg", srcdir);
        const char *tiffcp = getenv("TIFFCP");
        if (!tiffcp)
            tiffcp = "../tools/tiffcp";
        char script[1024];
        snprintf(script, sizeof(script), "%s/gen_bigtiff_from_jpeg.py", srcdir);
        char cmd[4096];
        snprintf(cmd, sizeof(cmd), "python3 \"%s\" \"%s\" \"%s\" \"%s\"",
                 script, jpeg, filepath, tiffcp);
        if (system(cmd) != 0)
        {
            fprintf(stderr, "Cannot generate %s\n", filepath);
            return 1;
        }
        generated = 1;
        in = TIFFOpen(filepath, "r");
        if (!in)
        {
            fprintf(stderr, "Cannot open %s\n", filepath);
            return 1;
        }
    }
    if (!TIFFIsBigTIFF(in))
    {
        fprintf(stderr, "Input is not BigTIFF\n");
        TIFFClose(in);
        return 1;
    }

    uint32_t width = 0, length = 0;
    uint16_t spp = 0, bps = 0, photo = 0;
    TIFFGetField(in, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(in, TIFFTAG_IMAGELENGTH, &length);
    TIFFGetField(in, TIFFTAG_SAMPLESPERPIXEL, &spp);
    TIFFGetField(in, TIFFTAG_BITSPERSAMPLE, &bps);
    TIFFGetField(in, TIFFTAG_PHOTOMETRIC, &photo);

    tsize_t scanline = TIFFScanlineSize(in);
    tdata_t buf = _TIFFmalloc(scanline);
    if (!buf)
    {
        fprintf(stderr, "Out of memory\n");
        TIFFClose(in);
        return 1;
    }

    const char *outfile = "roundtrip_big.tif";
    TIFF *out = TIFFOpen(outfile, "w8");
    if (!out)
    {
        fprintf(stderr, "Cannot create %s\n", outfile);
        _TIFFfree(buf);
        TIFFClose(in);
        return 1;
    }
    TIFFSetField(out, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(out, TIFFTAG_IMAGELENGTH, length);
    TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, spp);
    TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, bps);
    TIFFSetField(out, TIFFTAG_PHOTOMETRIC, photo);
    TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(out, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(out, 0));

    for (uint32_t y = 0; y < length; y++)
    {
        if (TIFFReadScanline(in, buf, y, 0) == -1 ||
            TIFFWriteScanline(out, buf, y, 0) == -1)
        {
            fprintf(stderr, "Read/write error at line %u\n", y);
            TIFFClose(out);
            _TIFFfree(buf);
            TIFFClose(in);
            unlink(outfile);
            return 1;
        }
    }
    _TIFFfree(buf);
    TIFFClose(in);
    TIFFClose(out);

    out = TIFFOpen(outfile, "r");
    if (!out)
    {
        fprintf(stderr, "Cannot reopen %s\n", outfile);
        return 1;
    }
    int isBig = TIFFIsBigTIFF(out);
    uint32_t rwidth = 0, rlength = 0;
    TIFFGetField(out, TIFFTAG_IMAGEWIDTH, &rwidth);
    TIFFGetField(out, TIFFTAG_IMAGELENGTH, &rlength);
    TIFFClose(out);
    unlink(outfile);
    if (generated)
        unlink(filepath);
    if (!isBig || rwidth != width || rlength != length)
        return 1;
    return 0;
}
