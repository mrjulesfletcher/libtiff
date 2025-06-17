#include "tif_config.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "tiffio.h"

#define THREADS 4

typedef struct
{
    const char *filename;
    unsigned long checksum;
    int ret;
} ThreadData;

static void *reader(void *arg)
{
    ThreadData *data = (ThreadData *)arg;
    TIFF *tif = TIFFOpen(data->filename, "r");
    if (!tif)
    {
        data->ret = 1;
        return NULL;
    }
    uint32_t width = 0, length = 0;
    if (!TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width) ||
        !TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &length))
    {
        TIFFClose(tif);
        data->ret = 1;
        return NULL;
    }
    tmsize_t scanline = TIFFScanlineSize(tif);
    unsigned char *buf = (unsigned char *)_TIFFmalloc(scanline);
    if (!buf)
    {
        TIFFClose(tif);
        data->ret = 1;
        return NULL;
    }
    data->checksum = 0;
    for (uint32_t row = 0; row < length; row++)
    {
        if (TIFFReadScanline(tif, buf, row, 0) < 0)
        {
            data->ret = 1;
            break;
        }
        for (tmsize_t i = 0; i < scanline; i++)
            data->checksum += buf[i];
    }
    _TIFFfree(buf);
    TIFFClose(tif);
    return NULL;
}

int main(void)
{
    const char *jpeg_rel = "images/TEST_JPEG.jpg";
    char *srcdir = getenv("srcdir");
    if (!srcdir)
        srcdir = ".";
    char jpeg_path[512];
    snprintf(jpeg_path, sizeof(jpeg_path), "%s/%s", srcdir, jpeg_rel);
    const char *script = "gen_bigtiff_from_jpeg.py";
    char script_path[512];
    snprintf(script_path, sizeof(script_path), "%s/%s", srcdir, script);
    const char *tiffcp = "../tools/tiffcp";
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "python3 %s %s mt_concurrent.tif %s",
             script_path, jpeg_path, tiffcp);
    if (system(cmd) != 0)
    {
        fprintf(stderr, "failed to generate TIFF\n");
        return 1;
    }

    pthread_t th[THREADS];
    ThreadData data[THREADS];
    for (int i = 0; i < THREADS; i++)
    {
        data[i].filename = "mt_concurrent.tif";
        data[i].checksum = 0;
        data[i].ret = 0;
        pthread_create(&th[i], NULL, reader, &data[i]);
    }
    int failed = 0;
    unsigned long checksum = 0;
    for (int i = 0; i < THREADS; i++)
    {
        pthread_join(th[i], NULL);
        if (data[i].ret)
            failed = 1;
        if (i == 0)
            checksum = data[i].checksum;
        else if (data[i].checksum != checksum)
            failed = 1;
    }
    unlink("mt_concurrent.tif");
    if (failed)
    {
        fprintf(stderr, "concurrent access failed\n");
        return 1;
    }
    return 0;
}
