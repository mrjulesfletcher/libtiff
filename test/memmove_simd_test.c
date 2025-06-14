#include "tiff_simd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    const size_t N = 1024;
    uint8_t *buf1 = (uint8_t *)malloc(N + 32);
    uint8_t *buf2 = (uint8_t *)malloc(N + 32);
    uint8_t *buf3 = (uint8_t *)malloc(N + 32);
    if (!buf1 || !buf2 || !buf3)
        return 1;
    for (size_t i = 0; i < N + 32; i++)
        buf1[i] = (uint8_t)(i & 0xff);
    memcpy(buf2, buf1, N + 32);
    memcpy(buf3, buf1, N + 32);

    /* dest < src overlap */
    tiff_memmove_u8(buf2 + 4, buf2, N);
    memmove(buf3 + 4, buf3, N);
    if (memcmp(buf2 + 4, buf3 + 4, N) != 0)
        return 1;

    memcpy(buf2, buf1, N + 32);
    memcpy(buf3, buf1, N + 32);

    /* dest > src overlap */
    tiff_memmove_u8(buf2, buf2 + 4, N);
    memmove(buf3, buf3 + 4, N);
    if (memcmp(buf2, buf3, N) != 0)
        return 1;

    free(buf1);
    free(buf2);
    free(buf3);
    return 0;
}
