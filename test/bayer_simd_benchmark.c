#include "tif_bayer.h"
#include "tiff_simd.h"
#include "tiffio.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static double elapsed_ms(struct timespec *s, struct timespec *e)
{
    return (e->tv_sec - s->tv_sec) * 1000.0 +
           (e->tv_nsec - s->tv_nsec) / 1000000.0;
}

static void run(size_t count)
{
    uint16_t *src = (uint16_t *)malloc(count * sizeof(uint16_t));
    uint16_t *dst = (uint16_t *)malloc(count * sizeof(uint16_t));
    size_t bytes = ((count + 1) / 2) * 3;
    uint8_t *buf = (uint8_t *)malloc(bytes);
    if (!src || !dst || !buf)
        exit(1);
    for (size_t i = 0; i < count; i++)
        src[i] = (uint16_t)(i & 0x0FFF);

    struct timespec s, e;
    clock_gettime(CLOCK_MONOTONIC, &s);
    TIFFPackRaw12(src, buf, count, 0);
    clock_gettime(CLOCK_MONOTONIC, &e);
    double pack_ms = elapsed_ms(&s, &e);

    clock_gettime(CLOCK_MONOTONIC, &s);
    TIFFUnpackRaw12(buf, dst, count, 0);
    clock_gettime(CLOCK_MONOTONIC, &e);
    double unpack_ms = elapsed_ms(&s, &e);

    printf("pack %.2f ms, unpack %.2f ms\n", pack_ms, unpack_ms);

    free(src);
    free(dst);
    free(buf);
}

int main(void)
{
    TIFFInitSIMD();
    size_t count = 1 << 20; /* 1M samples */
    int neon = TIFFUseNEON();

#if defined(HAVE_NEON) && defined(__ARM_NEON)
    TIFFSetUseNEON(0);
#endif
    printf("scalar:\n");
    run(count);

#if defined(HAVE_NEON) && defined(__ARM_NEON)
    TIFFSetUseNEON(neon);
    if (TIFFUseNEON())
    {
        printf("NEON:\n");
        run(count);
    }
#endif
    return 0;
}
