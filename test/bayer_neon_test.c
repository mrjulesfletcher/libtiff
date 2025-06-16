#include "tif_bayer.h"
#include "tiffio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t bytes_for_depth(size_t count, int bits)
{
    switch(bits)
    {
    case 10: return ((count + 3) / 4) * 5;
    case 14: return ((count + 3) / 4) * 7;
    default: return 0;
    }
}

static int test_depth(int bits)
{
    const size_t count = 32;
    uint16_t *src = (uint16_t*)malloc(count * sizeof(uint16_t));
    uint16_t *ref = (uint16_t*)malloc(count * sizeof(uint16_t));
    uint16_t *simd = (uint16_t*)malloc(count * sizeof(uint16_t));
    if(!src || !ref || !simd) return 1;
    for(size_t i=0;i<count;i++)
        src[i] = (uint16_t)((i*5+7) & ((1U<<bits)-1));
    size_t bytes = bytes_for_depth(count, bits);
    uint8_t *buf_ref = (uint8_t*)malloc(bytes);
    uint8_t *buf_simd = (uint8_t*)malloc(bytes);
    if(!buf_ref || !buf_simd) return 1;

    TIFFInitSIMD();
    int neon = TIFFUseNEON();
    TIFFSetUseNEON(0);
    if(bits==10)
    {
        TIFFPackRaw10(src, buf_ref, count, 0);
        TIFFUnpackRaw10(buf_ref, ref, count, 0);
    }
    else
    {
        TIFFPackRaw14(src, buf_ref, count, 0);
        TIFFUnpackRaw14(buf_ref, ref, count, 0);
    }
    TIFFSetUseNEON(neon);
    if(bits==10)
    {
        TIFFPackRaw10(src, buf_simd, count, 0);
        TIFFUnpackRaw10(buf_simd, simd, count, 0);
    }
    else
    {
        TIFFPackRaw14(src, buf_simd, count, 0);
        TIFFUnpackRaw14(buf_simd, simd, count, 0);
    }

    int ret = 0;
    if(memcmp(buf_ref, buf_simd, bytes) != 0 ||
       memcmp(ref, simd, count*sizeof(uint16_t)) != 0)
        ret = 1;
    free(src); free(ref); free(simd); free(buf_ref); free(buf_simd);
    return ret;
}

int main(void)
{
    if(test_depth(10)) { fprintf(stderr, "10-bit mismatch\n"); return 1; }
    if(test_depth(14)) { fprintf(stderr, "14-bit mismatch\n"); return 1; }
    return 0;
}
