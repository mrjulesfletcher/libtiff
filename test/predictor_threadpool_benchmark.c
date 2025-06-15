#include "strip_neon.h"
#include "tiff_threadpool.h"
#include "tiffiop.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef USE_IO_URING
extern tmsize_t _tiffUringWriteProc(thandle_t fd, void *buf, tmsize_t size);
extern void _tiffUringSetAsync(TIFF *tif, int enable);
extern void _tiffUringWait(TIFF *tif);
#endif

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
        if (!dst)
            continue;
#ifdef USE_IO_URING
        _tiffUringWriteProc((thandle_t)TIFFFileno(t->tif), dst,
                            (tmsize_t)out_size);
#endif
        free(dst);
    }
}

static double elapsed_ms(struct timespec *s, struct timespec *e)
{
    return (e->tv_sec - s->tv_sec) * 1000.0 +
           (e->tv_nsec - s->tv_nsec) / 1000000.0;
}

int main(int argc, char **argv)
{
    int threads = 4;
    size_t loops = 50;
    if (argc > 1)
        threads = atoi(argv[1]);
    if (argc > 2)
        loops = (size_t)atoi(argv[2]);

    TIFFThreadPool *tp = _TIFFThreadPoolInit(threads);

    const char *filename = "predictor_threadpool.tmp";
    TIFF *tif = TIFFOpen(filename, "w+");
    if (!tif)
    {
        fprintf(stderr, "cannot create %s\n", filename);
        return 1;
    }
#ifdef USE_IO_URING
    _tiffUringSetAsync(tif, 1);
#endif

    const uint32_t width = 512, height = 512;
    size_t count = (size_t)width * height;
    uint16_t *buf = (uint16_t *)malloc(count * sizeof(uint16_t));
    if (!buf)
        return 1;
    for (size_t i = 0; i < count; i++)
        buf[i] = (uint16_t)(i & 0x0FFF);

    Task *tasks = (Task *)calloc(threads, sizeof(Task));
    if (!tasks)
        return 1;
    for (int t = 0; t < threads; t++)
    {
        tasks[t].src = buf;
        tasks[t].width = width;
        tasks[t].height = height;
        tasks[t].loops = loops;
        tasks[t].tif = tif;
        _TIFFThreadPoolSubmit(tp, bench_task, &tasks[t]);
    }

    struct timespec s, e;
    clock_gettime(CLOCK_MONOTONIC, &s);
    _TIFFThreadPoolWait(tp);
#ifdef USE_IO_URING
    _tiffUringWait(tif);
#endif
    clock_gettime(CLOCK_MONOTONIC, &e);

    printf("predictor+pack with %d threads (%zu loops each): %.2f ms\n",
           threads, loops, elapsed_ms(&s, &e));

    free(tasks);
    free(buf);
    _TIFFThreadPoolShutdown(tp);
    TIFFClose(tif);
#ifdef USE_IO_URING
    unlink(filename);
#else
    (void)filename;
#endif
    return 0;
}
