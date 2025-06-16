#include "libport.h"
#include "tif_config.h"
#include "tif_bayer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

static double now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static void usage(void)
{
    fprintf(stderr, "usage: bayerbench [loops]\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    size_t pixels = 1 << 20; /* 1M pixels */
    /* optional command line argument: number of iterations */
    int loops = 100; /* number of pack/unpack iterations */
    if (argc > 1)
    {
        char *endptr = NULL;
        errno = 0;
        long val = strtol(argv[1], &endptr, 10);
        if (errno != 0 || endptr == argv[1] || *endptr != '\0' || val <= 0 ||
            val > INT_MAX)
            usage();
        loops = (int)val;
    }

    uint16_t *src = (uint16_t *)malloc(pixels * sizeof(uint16_t));
    uint8_t *buf = (uint8_t *)malloc(pixels * 3 / 2);
    uint16_t *dst = (uint16_t *)malloc(pixels * sizeof(uint16_t));

    if (!src || !buf || !dst)
    {
        fprintf(stderr, "allocation failure\n");
        return 1;
    }

    /* seed RNG to provide varied input */
    srand((unsigned)time(NULL));
    for (size_t i = 0; i < pixels; i++)
        src[i] = rand() & 0xFFF;

    double t0 = now();
    for (int i = 0; i < loops; i++)
        TIFFPackRaw12(src, buf, pixels, 0);
    double t1 = now();

    for (int i = 0; i < loops; i++)
        TIFFUnpackRaw12(buf, dst, pixels, 0);
    double t2 = now();

    printf("pack:   %.2f MPix/s\n", (pixels * loops / 1e6) / (t1 - t0));
    printf("unpack: %.2f MPix/s\n", (pixels * loops / 1e6) / (t2 - t1));

    free(src);
    free(buf);
    free(dst);
    return 0;
}
