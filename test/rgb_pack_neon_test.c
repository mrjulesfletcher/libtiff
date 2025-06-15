#include "rgb_neon.h"
#include "tiffio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int test_rgb24(void)
{
    const size_t N = 32;
    uint8_t *src = (uint8_t *)malloc(N * 3);
    uint32_t *ref = (uint32_t *)malloc(N * sizeof(uint32_t));
    uint32_t *simd = (uint32_t *)malloc(N * sizeof(uint32_t));
    if (!src || !ref || !simd)
        return 1;
    for (size_t i = 0; i < N * 3; i++)
        src[i] = (uint8_t)(i * 3 + 1);
    int neon = TIFFUseNEON();
    TIFFSetUseNEON(0);
    TIFFPackRGB24(src, ref, N);
    TIFFSetUseNEON(neon);
    TIFFPackRGB24(src, simd, N);
    int ret = memcmp(ref, simd, N * sizeof(uint32_t)) != 0;
    free(src);
    free(ref);
    free(simd);
    return ret;
}

static int test_rgba32(void)
{
    const size_t N = 32;
    uint8_t *src = (uint8_t *)malloc(N * 4);
    uint32_t *ref = (uint32_t *)malloc(N * sizeof(uint32_t));
    uint32_t *simd = (uint32_t *)malloc(N * sizeof(uint32_t));
    if (!src || !ref || !simd)
        return 1;
    for (size_t i = 0; i < N * 4; i++)
        src[i] = (uint8_t)(i * 5 + 7);
    int neon = TIFFUseNEON();
    TIFFSetUseNEON(0);
    TIFFPackRGBA32(src, ref, N);
    TIFFSetUseNEON(neon);
    TIFFPackRGBA32(src, simd, N);
    int ret = memcmp(ref, simd, N * sizeof(uint32_t)) != 0;
    free(src);
    free(ref);
    free(simd);
    return ret;
}

int main(void)
{
    TIFFInitSIMD();
    if (test_rgb24())
    {
        fprintf(stderr, "rgb24 mismatch\n");
        return 1;
    }
    if (test_rgba32())
    {
        fprintf(stderr, "rgba32 mismatch\n");
        return 1;
    }
    return 0;
}
