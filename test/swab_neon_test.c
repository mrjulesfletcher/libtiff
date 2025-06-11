#include "tiffio.h"
#include <stdio.h>
#include <stdlib.h>

static int test_short(void)
{
    const int N = 1024;
    uint16_t *buf = (uint16_t*)malloc(N * sizeof(uint16_t));
    uint16_t *ref = (uint16_t*)malloc(N * sizeof(uint16_t));
    if(!buf || !ref) return 1;
    for(int i=0;i<N;i++)
    {
        buf[i] = (uint16_t)i;
        uint16_t v = (uint16_t)i;
        ref[i] = (uint16_t)((v >> 8) | (v << 8));
    }
    TIFFSwabArrayOfShort(buf, N);
    for(int i=0;i<N;i++)
    {
        if(buf[i] != ref[i])
        {
            fprintf(stderr, "Short mismatch at %d\n", i);
            free(buf); free(ref); return 1;
        }
    }
    free(buf); free(ref); return 0;
}

static int test_long(void)
{
    const int N = 1024;
    uint32_t *buf = (uint32_t*)malloc(N * sizeof(uint32_t));
    uint32_t *ref = (uint32_t*)malloc(N * sizeof(uint32_t));
    if(!buf || !ref) return 1;
    for(int i=0;i<N;i++)
    {
        buf[i] = (uint32_t)(0x01020300u + i);
        uint32_t v = buf[i];
        ref[i] = ((v & 0x000000FFU) << 24) | ((v & 0x0000FF00U) << 8) |
                 ((v & 0x00FF0000U) >> 8) | ((v & 0xFF000000U) >> 24);
    }
    TIFFSwabArrayOfLong(buf, N);
    for(int i=0;i<N;i++)
    {
        if(buf[i] != ref[i])
        {
            fprintf(stderr, "Long mismatch at %d\n", i);
            free(buf); free(ref); return 1;
        }
    }
    free(buf); free(ref); return 0;
}

int main(void)
{
    if(test_short()) return 1;
    if(test_long()) return 1;
    return 0;
}
