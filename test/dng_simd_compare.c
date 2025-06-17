#include "tiffio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    TIFFInitSIMD();
    uint32_t *ref = NULL;
    uint32_t *simd = NULL;
    size_t n = 0;
    const char *rel = "images/TEST_CINEPI_LIBTIFF_DNG.dng";
    char *srcdir = getenv("srcdir");
    if (!srcdir)
        srcdir = ".";
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", srcdir, rel);

    TIFF *tif = TIFFOpen(path, "r");
    if (!tif)
    {
        /* If the DNG sample is unavailable or unreadable, fall back to a JPEG test */
        const char *jpeg_rel = "images/TEST_JPEG.jpg";
        char jpath[512];
        snprintf(jpath, sizeof(jpath), "%s/%s", srcdir, jpeg_rel);
        const char *script = "gen_bigtiff_from_jpeg.py";
        char spath[512];
        snprintf(spath, sizeof(spath), "%s/%s", srcdir, script);
        const char *tiffcp = "../tools/tiffcp";
        if (system(NULL) == 0)
            return 1;
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "python3 %s %s jpeg_temp.tif %s", spath, jpath, tiffcp);
        if (system(cmd) != 0)
            return 1;
        tif = TIFFOpen("jpeg_temp.tif", "r");
        if (!tif)
            return 1;
    }
    uint32_t w = 0, h = 0;
    if (!TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w) ||
        !TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h))
    {
        TIFFClose(tif);
        return 1;
    }
    n = (size_t)w * h;
    ref = (uint32_t *)malloc(n * sizeof(uint32_t));
    simd = (uint32_t *)malloc(n * sizeof(uint32_t));
    if (!ref || !simd)
    {
        TIFFClose(tif);
        return 1;
    }

    int neon = TIFFUseNEON();
    int sse = TIFFUseSSE41();

    if (neon)
        TIFFSetUseNEON(0);
    if (sse)
        TIFFSetUseSSE41(0);
    if (!TIFFReadRGBAImage(tif, w, h, ref, 0))
    {
        /* reading DNG failed, try JPEG bigtiff fallback */
        TIFFClose(tif);
        const char *jpeg_rel = "images/TEST_JPEG.jpg";
        char jpath[512];
        snprintf(jpath, sizeof(jpath), "%s/%s", srcdir, jpeg_rel);
        const char *script = "gen_bigtiff_from_jpeg.py";
        char spath[512];
        snprintf(spath, sizeof(spath), "%s/%s", srcdir, script);
        const char *tiffcp = "../tools/tiffcp";
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "python3 %s %s jpeg_temp.tif %s", spath, jpath, tiffcp);
        if (system(cmd) != 0)
        {
            free(ref);
            free(simd);
            return 1;
        }
        tif = TIFFOpen("jpeg_temp.tif", "r");
        if (!tif)
        {
            free(ref);
            free(simd);
            return 1;
        }
        if (!TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w) ||
            !TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h))
        {
            TIFFClose(tif);
            free(ref);
            free(simd);
            return 1;
        }
        n = (size_t)w * h;
        ref = (uint32_t *)realloc(ref, n * sizeof(uint32_t));
        simd = (uint32_t *)realloc(simd, n * sizeof(uint32_t));
        if (!ref || !simd)
        {
            TIFFClose(tif);
            free(ref);
            free(simd);
            return 1;
        }
        if (!TIFFReadRGBAImage(tif, w, h, ref, 0))
        {
            TIFFClose(tif);
            free(ref);
            free(simd);
            return 1;
        }
    }
    if (neon)
        TIFFSetUseNEON(1);
    if (sse)
        TIFFSetUseSSE41(1);
    if (!TIFFReadRGBAImage(tif, w, h, simd, 0))
    {
        TIFFClose(tif);
        free(ref);
        free(simd);
        return 1;
    }
    TIFFClose(tif);

    int ret = memcmp(ref, simd, n * sizeof(uint32_t)) != 0;
    if (ret)
        fprintf(stderr, "SIMD output differs from scalar\n");

    remove("jpeg_temp.tif");

    free(ref);
    free(simd);
    return ret;
}
