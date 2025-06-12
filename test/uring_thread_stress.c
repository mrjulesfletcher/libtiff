#include "tif_config.h"
#include "tiffio.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define THREADS 4

static void *worker(void *arg)
{
    int idx = (int)(intptr_t)arg;
    char filename[32];
    snprintf(filename, sizeof(filename), "uring_mt_%d.tif", idx);

    TIFF *tif = TIFFOpen(filename, "w");
    if (!tif)
        return (void *)1;
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, 1);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, 1);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    unsigned char buf[1] = {(unsigned char)idx};
    if (TIFFWriteEncodedStrip(tif, 0, buf, 1) != 1)
    {
        TIFFClose(tif);
        return (void *)1;
    }
    TIFFClose(tif);

    tif = TIFFOpen(filename, "r");
    if (!tif)
        return (void *)1;
    unsigned char rbuf[1] = {0};
    if (TIFFReadEncodedStrip(tif, 0, rbuf, 1) != 1 || rbuf[0] != buf[0])
    {
        TIFFClose(tif);
        return (void *)1;
    }
    TIFFClose(tif);
    unlink(filename);
    return NULL;
}

int main(void)
{
    pthread_t th[THREADS];
    for (int i = 0; i < THREADS; i++)
        pthread_create(&th[i], NULL, worker, (void *)(intptr_t)i);
    int failed = 0;
    for (int i = 0; i < THREADS; i++)
    {
        void *ret = NULL;
        pthread_join(th[i], &ret);
        if (ret)
            failed = 1;
    }
    return failed;
}
