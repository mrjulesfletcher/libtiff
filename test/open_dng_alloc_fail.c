#include "failalloc.h"
#include "tiffio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

int main(void)
{
    const char *dng_rel = "images/TEST_CINEPI_LIBTIFF_DNG.dng";
    char *srcdir = getenv("srcdir");
    if (!srcdir)
        srcdir = ".";
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", srcdir, dng_rel);

    /* Simulate failure on first malloc when opening */
    setenv("FAIL_MALLOC_COUNT", "1", 1);
    failalloc_reset_from_env();
    TIFF *tif = TIFFOpen(path, "r");
    if (tif)
    {
        fprintf(stderr, "Expected TIFFOpen to fail with OOM\n");
        TIFFClose(tif);
        return 1;
    }

    /* Open successfully without failure */
    unsetenv("FAIL_MALLOC_COUNT");
    failalloc_reset_from_env();
    tif = TIFFOpen(path, "r");
    if (!tif)
    {
        fprintf(stderr, "Cannot open %s\n", path);
        return 1;
    }
    TIFFClose(tif);
    return 0;
}
