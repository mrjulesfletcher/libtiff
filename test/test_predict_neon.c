#include "tif_predict_neon.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>

static void scalar_hordiff16(uint16_t *line, size_t width)
{
    if (width <= 1)
        return;
    uint16_t prev = line[0];
    for (size_t i = 1; i < width; ++i)
    {
        uint16_t cur = line[i];
        line[i] = (uint16_t)(cur - prev);
        prev = cur;
    }
}

static void scalar_horacc16(uint16_t *line, size_t width)
{
    if (width <= 1)
        return;
    uint16_t prev = line[0];
    for (size_t i = 1; i < width; ++i)
    {
        line[i] = (uint16_t)(line[i] + prev);
        prev = line[i];
    }
}

static void scalar_median_diff16(const uint16_t *cur, const uint16_t *prev, uint16_t *dst, size_t width)
{
    if (width == 0)
        return;
    dst[0] = (uint16_t)(cur[0] - prev[0]);
    for (size_t i = 1; i < width; ++i)
    {
        uint16_t A = cur[i - 1];
        uint16_t B = prev[i];
        uint16_t C = prev[i - 1];
        uint16_t guess = (uint16_t)(A + B - C);
        uint16_t minAB = A < B ? A : B;
        uint16_t maxAB = A > B ? A : B;
        uint16_t med = guess < minAB ? minAB : (guess > maxAB ? maxAB : guess);
        dst[i] = (uint16_t)(cur[i] - med);
    }
}

static void scalar_median_acc16(const uint16_t *prev, uint16_t *cur, size_t width)
{
    if (width == 0)
        return;
    cur[0] = (uint16_t)(cur[0] + prev[0]);
    for (size_t i = 1; i < width; ++i)
    {
        uint16_t A = cur[i - 1];
        uint16_t B = prev[i];
        uint16_t C = prev[i - 1];
        uint16_t guess = (uint16_t)(A + B - C);
        uint16_t minAB = A < B ? A : B;
        uint16_t maxAB = A > B ? A : B;
        uint16_t med = guess < minAB ? minAB : (guess > maxAB ? maxAB : guess);
        cur[i] = (uint16_t)(cur[i] + med);
    }
}

int main(void)
{
    const size_t WIDTH = 32;
    uint16_t line[WIDTH];
    uint16_t line_ref[WIDTH];
    uint16_t prev[WIDTH];
    uint16_t diff[WIDTH];
    uint16_t diff_ref[WIDTH];

    for (size_t i = 0; i < WIDTH; ++i)
    {
        line[i] = (uint16_t)(i * 3 + 1);
        line_ref[i] = line[i];
        prev[i] = (uint16_t)(i * 2 + 2);
    }

    memcpy(diff, line, sizeof(line));
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    TIFFPredictHorDiff16_NEON(diff, WIDTH);
#else
    scalar_hordiff16(diff, WIDTH);
#endif
    memcpy(diff_ref, line_ref, sizeof(line_ref));
    scalar_hordiff16(diff_ref, WIDTH);
    if (memcmp(diff, diff_ref, sizeof(diff)) != 0)
    {
        printf("hor diff mismatch\n");
        return 1;
    }

#if defined(HAVE_NEON) && defined(__ARM_NEON)
    TIFFPredictHorAcc16_NEON(diff, WIDTH);
#else
    scalar_horacc16(diff, WIDTH);
#endif
    scalar_horacc16(diff_ref, WIDTH);
    if (memcmp(diff, diff_ref, sizeof(diff)) != 0)
    {
        printf("hor acc mismatch\n");
        return 1;
    }

    scalar_median_diff16(line, prev, diff_ref, WIDTH);
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    TIFFPredictMedianDiff16_NEON(line, prev, diff, WIDTH);
#else
    scalar_median_diff16(line, prev, diff, WIDTH);
#endif
    if (memcmp(diff, diff_ref, sizeof(diff)) != 0)
    {
        printf("median diff mismatch\n");
        return 1;
    }

    memcpy(line_ref, diff_ref, sizeof(diff_ref));
    scalar_median_acc16(prev, line_ref, WIDTH);
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    memcpy(line, diff, sizeof(diff));
    scalar_median_acc16(prev, line, WIDTH); /* NEON decode not implemented */
#else
    memcpy(line, diff, sizeof(diff));
    scalar_median_acc16(prev, line, WIDTH);
#endif
    if (memcmp(line, line_ref, sizeof(line)) != 0)
    {
        printf("median acc mismatch\n");
        return 1;
    }

    return 0;
}
