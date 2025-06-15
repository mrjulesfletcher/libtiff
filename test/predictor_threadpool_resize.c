#include "strip_neon.h"
#include "tiff_threadpool.h"
#include "tiffiop.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct
{
    const uint16_t *src;
    uint32_t width;
    uint32_t height;
    size_t loops;
    TIFF *tif;
} Task;

static void bench_task(void *arg)
{
    Task *t = (Task *)arg;
    for (size_t i = 0; i < t->loops; i++)
    {
        size_t out_size = 0;
        uint8_t *dst = TIFFAssembleStripNEON(NULL, t->src, t->width, t->height,
                                             1, 0, &out_size);
        free(dst);
    }
}

int main(void)
{
    TIFF *tif = TIFFOpen("resize.tmp", "w+");
    if (!tif)
        return 1;
    TIFFSetThreadPoolSize(tif, 1, 256);

    const int tasks = 8;
    const uint32_t width = 256, height = 256;
    size_t count = (size_t)width * height;
    uint16_t *buf = (uint16_t *)malloc(count * sizeof(uint16_t));
    if (!buf)
        return 1;
    for (size_t i = 0; i < count; i++)
        buf[i] = (uint16_t)(i & 0x0FFF);

    Task *tarray = (Task *)calloc(tasks, sizeof(Task));
    if (!tarray)
        return 1;
    for (int t = 0; t < tasks; t++)
    {
        tarray[t].src = buf;
        tarray[t].width = width;
        tarray[t].height = height;
        tarray[t].loops = 20;
        tarray[t].tif = tif;
        _TIFFThreadPoolSubmit(tif->tif_threadpool, bench_task, &tarray[t]);
    }

    TIFFSetThreadPoolSize(tif, 0, 256);
    int new_workers = TIFFGetThreadCount(tif);

    for (int t = 0; t < tasks; t++)
        _TIFFThreadPoolSubmit(tif->tif_threadpool, bench_task, &tarray[t]);

    _TIFFThreadPoolWait(tif->tif_threadpool);
    int ret = (new_workers <= 1);
    free(tarray);
    free(buf);
    TIFFClose(tif);
    unlink("resize.tmp");
    return ret;
}
