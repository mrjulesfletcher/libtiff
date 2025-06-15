#include "reverse_bits_neon.h"
#include "tiffio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void reverse_bits_ref(uint8_t *buf, size_t n)
{
    const unsigned char *tbl = TIFFGetBitRevTable(1);
    for (size_t i = 0; i < n; i++)
        buf[i] = tbl[buf[i]];
}

int main(void)
{
    const size_t N = 1024;
    uint8_t *in1 = (uint8_t *)malloc(N);
    uint8_t *in2 = (uint8_t *)malloc(N);
    uint8_t *orig = (uint8_t *)malloc(N);
    if (!in1 || !in2 || !orig)
        return 1;
    for (size_t i = 0; i < N; i++)
    {
        orig[i] = (uint8_t)(i & 0xff);
        in1[i] = orig[i];
        in2[i] = orig[i];
    }

    reverse_bits_ref(in1, N);
    TIFFReverseBitsNeon(in2, (tmsize_t)N);
    if (memcmp(in1, in2, N) != 0)
    {
        fprintf(stderr, "TIFFReverseBitsNeon mismatch\n");
        return 1;
    }

    memcpy(in2, orig, N);
    TIFFReverseBits(in2, (tmsize_t)N);
    if (memcmp(in1, in2, N) != 0)
    {
        fprintf(stderr, "TIFFReverseBits dispatch mismatch\n");
        return 1;
    }

    free(in1);
    free(in2);
    free(orig);
    return 0;
}
