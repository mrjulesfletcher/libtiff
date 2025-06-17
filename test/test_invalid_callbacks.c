#include "tiffio.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

static char g_error[256];
static char g_warning[256];

static void myErrorHandler(thandle_t fd, const char *module, const char *fmt,
                           va_list ap)
{
    (void)fd;
    (void)module;
    vsnprintf(g_error, sizeof(g_error), fmt, ap);
}

static void myWarningHandler(thandle_t fd, const char *module, const char *fmt,
                             va_list ap)
{
    (void)fd;
    (void)module;
    vsnprintf(g_warning, sizeof(g_warning), fmt, ap);
}

static int copy_file(const char *src, const char *dst)
{
    FILE *fs = fopen(src, "rb");
    if (!fs)
        return 0;
    FILE *fd = fopen(dst, "wb");
    if (!fd)
    {
        fclose(fs);
        return 0;
    }
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fs)) > 0)
    {
        if (fwrite(buf, 1, n, fd) != n)
        {
            fclose(fs);
            fclose(fd);
            return 0;
        }
    }
    fclose(fs);
    fclose(fd);
    return 1;
}

int main(void)
{
    const char *srcdir = getenv("srcdir");
    if (!srcdir)
        srcdir = ".";
    char path[1024];
    char tmp[1024];
    TIFF *tif;
    int ret = 0;

    TIFFErrorHandlerExt prevErr = TIFFSetErrorHandlerExt(myErrorHandler);
    TIFFErrorHandlerExt prevWarn = TIFFSetWarningHandlerExt(myWarningHandler);

    snprintf(path, sizeof(path), "%s/images/TEST_JPEG.jpg", srcdir);
    snprintf(tmp, sizeof(tmp), "invalid_jpeg.tif");
    if (!copy_file(path, tmp))
    {
        fprintf(stderr, "Cannot copy JPEG sample\n");
        return 1;
    }
    tif = TIFFOpen(tmp, "r");
    if (tif)
    {
        fprintf(stderr, "Unexpectedly opened JPEG as TIFF\n");
        TIFFClose(tif);
        ret = 1;
    }
    if (strstr(g_error, "Not a TIFF") == NULL)
    {
        fprintf(stderr, "Did not get expected error for JPEG: %s\n", g_error);
        ret = 1;
    }
    g_error[0] = '\0';

    snprintf(path, sizeof(path), "%s/images/TEST_CINEPI_LIBTIFF_DNG.dng",
             srcdir);
    snprintf(tmp, sizeof(tmp), "invalid_dng.tif");
    if (!copy_file(path, tmp))
    {
        fprintf(stderr, "Cannot copy DNG sample\n");
        return 1;
    }
    FILE *f = fopen(tmp, "r+b");
    if (!f)
    {
        perror("fopen");
        return 1;
    }
    if (fseek(f, 298, SEEK_SET) != 0)
    {
        fclose(f);
        fprintf(stderr, "fseek failed\n");
        return 1;
    }
    unsigned char off[4] = {8, 0, 0, 0};
    if (fwrite(off, 1, 4, f) != 4)
    {
        fclose(f);
        fprintf(stderr, "fwrite failed\n");
        return 1;
    }
    fclose(f);

    tif = TIFFOpen(tmp, "r");
    if (tif)
    {
        /* Try to read the next directory to trigger loop detection */
        (void)TIFFReadDirectory(tif);
        TIFFClose(tif);
    }
    if (strstr(g_warning, "IFD looping") == NULL)
    {
        fprintf(stderr, "Did not get expected warning for DNG: %s\n",
                g_warning);
        ret = 1;
    }
    TIFFSetErrorHandlerExt(prevErr);
    TIFFSetWarningHandlerExt(prevWarn);
    unlink("invalid_jpeg.tif");
    unlink("invalid_dng.tif");
    return ret;
}
