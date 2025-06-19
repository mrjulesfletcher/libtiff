#include "tiff_simd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double elapsed_ms(struct timespec *s, struct timespec *e)
{
    return (e->tv_sec - s->tv_sec) * 1000.0 +
           (e->tv_nsec - s->tv_nsec) / 1000000.0;
}

int main(int argc, char **argv)
{
    int loops = 100;
    if (argc > 1)
        loops = atoi(argv[1]);
    const size_t N = 1 << 20; /* 1 MiB */
    uint8_t *buf = malloc(N);
    if (!buf)
        return 1;
    for (size_t i = 0; i < N; i++)
        buf[i] = (uint8_t)(i * 13 + 7);
    struct timespec s, e;
    uint64_t h = 0;
    clock_gettime(CLOCK_MONOTONIC, &s);
    for (int i = 0; i < loops; i++)
        h = tiff_pmull_hash(h, buf, N);
    clock_gettime(CLOCK_MONOTONIC, &e);
    double ms = elapsed_ms(&s, &e);
    double throughput = (double)N * loops / (ms * 1e6); /* MB/s */
    printf("hash=0x%016llx\n", (unsigned long long)h);
    printf("throughput: %.2f MB/s\n", throughput);
    free(buf);
    return 0;
}
