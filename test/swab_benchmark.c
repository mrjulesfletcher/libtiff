#include "tiffio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void scalar_swab_short(uint16_t *wp, size_t n)
{
    while (n--)
    {
        unsigned char *cp = (unsigned char *)wp;
        unsigned char t = cp[1];
        cp[1] = cp[0];
        cp[0] = t;
        wp++;
    }
}

static void scalar_swab_long(uint32_t *lp, size_t n)
{
    while (n--)
    {
        unsigned char *cp = (unsigned char *)lp;
        unsigned char t = cp[3];
        cp[3] = cp[0];
        cp[0] = t;
        t = cp[2];
        cp[2] = cp[1];
        cp[1] = t;
        lp++;
    }
}

static void scalar_swab_long8(uint64_t *lp, size_t n)
{
    while (n--)
    {
        unsigned char *cp = (unsigned char *)lp;
        unsigned char t = cp[7];
        cp[7] = cp[0];
        cp[0] = t;
        t = cp[6];
        cp[6] = cp[1];
        cp[1] = t;
        t = cp[5];
        cp[5] = cp[2];
        cp[2] = t;
        t = cp[4];
        cp[4] = cp[3];
        cp[3] = t;
        lp++;
    }
}

static double elapsed_ms(struct timespec *s, struct timespec *e)
{
    return (e->tv_sec - s->tv_sec) * 1000.0 +
           (e->tv_nsec - s->tv_nsec) / 1000000.0;
}

int main(void)
{
    const size_t N = 1 << 16;
    uint16_t *buf16 = malloc(N * sizeof(uint16_t));
    uint16_t *src16 = malloc(N * sizeof(uint16_t));
    uint32_t *buf32 = malloc(N * sizeof(uint32_t));
    uint32_t *src32 = malloc(N * sizeof(uint32_t));
    uint64_t *buf64 = malloc(N * sizeof(uint64_t));
    uint64_t *src64 = malloc(N * sizeof(uint64_t));
    if (!buf16 || !src16 || !buf32 || !src32 || !buf64 || !src64)
        return 1;
    for (size_t i = 0; i < N; i++)
    {
        src16[i] = (uint16_t)i;
        src32[i] = (uint32_t)(0x01020300u + i);
        src64[i] = (uint64_t)(0x0102030405060700ULL + i);
    }
    struct timespec s, e;

    memcpy(buf16, src16, N * sizeof(uint16_t));
    clock_gettime(CLOCK_MONOTONIC, &s);
    TIFFSwabArrayOfShort(buf16, N);
    clock_gettime(CLOCK_MONOTONIC, &e);
    printf("TIFFSwabArrayOfShort: %.3f ms\n", elapsed_ms(&s, &e));

    memcpy(buf16, src16, N * sizeof(uint16_t));
    clock_gettime(CLOCK_MONOTONIC, &s);
    scalar_swab_short(buf16, N);
    clock_gettime(CLOCK_MONOTONIC, &e);
    printf("scalar_swab_short: %.3f ms\n", elapsed_ms(&s, &e));

    memcpy(buf32, src32, N * sizeof(uint32_t));
    clock_gettime(CLOCK_MONOTONIC, &s);
    TIFFSwabArrayOfLong(buf32, N);
    clock_gettime(CLOCK_MONOTONIC, &e);
    printf("TIFFSwabArrayOfLong: %.3f ms\n", elapsed_ms(&s, &e));

    memcpy(buf32, src32, N * sizeof(uint32_t));
    clock_gettime(CLOCK_MONOTONIC, &s);
    scalar_swab_long(buf32, N);
    clock_gettime(CLOCK_MONOTONIC, &e);
    printf("scalar_swab_long: %.3f ms\n", elapsed_ms(&s, &e));

    memcpy(buf64, src64, N * sizeof(uint64_t));
    clock_gettime(CLOCK_MONOTONIC, &s);
    TIFFSwabArrayOfLong8(buf64, N);
    clock_gettime(CLOCK_MONOTONIC, &e);
    printf("TIFFSwabArrayOfLong8: %.3f ms\n", elapsed_ms(&s, &e));

    memcpy(buf64, src64, N * sizeof(uint64_t));
    clock_gettime(CLOCK_MONOTONIC, &s);
    scalar_swab_long8(buf64, N);
    clock_gettime(CLOCK_MONOTONIC, &e);
    printf("scalar_swab_long8: %.3f ms\n", elapsed_ms(&s, &e));

    free(buf16);
    free(src16);
    free(buf32);
    free(src32);
    free(buf64);
    free(src64);
    return 0;
}
