#include "tif_predict_neon.h"

#if defined(HAVE_NEON) && defined(__ARM_NEON)
#include <arm_neon.h>
#endif

void TIFFPredictHorDiff16_NEON(uint16_t *line, size_t width)
{
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    if (width <= 1)
        return;
    uint16_t prev = line[0];
    size_t i = 1;
    size_t aligned_end = 1 + ((width - 1) & ~7U);
    uint16x8_t prev_vec = vdupq_n_u16(prev);
    for (; i < aligned_end; i += 8)
    {
        uint16x8_t cur = vld1q_u16(line + i);
        uint16x8_t shifted = vextq_u16(prev_vec, cur, 7);
        uint16x8_t diff = vsubq_u16(cur, shifted);
        vst1q_u16(line + i, diff);
        prev_vec = cur;
        prev = vgetq_lane_u16(cur, 7);
    }
    for (; i < width; ++i)
    {
        uint16_t cur = line[i];
        line[i] = (uint16_t)(cur - prev);
        prev = cur;
    }
#else
    (void)line;
    (void)width;
#endif
}

void TIFFPredictHorAcc16_NEON(uint16_t *line, size_t width)
{
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    if (width <= 1)
        return;
    uint16_t prev = line[0];
    size_t i = 1;
    size_t aligned_end = 1 + ((width - 1) & ~7U);
    uint16x8_t prev_vec = vdupq_n_u16(prev);
    for (; i < aligned_end; i += 8)
    {
        uint16x8_t cur = vld1q_u16(line + i);
        uint16x8_t shifted = vextq_u16(prev_vec, cur, 7);
        uint16x8_t sum = vaddq_u16(cur, shifted);
        vst1q_u16(line + i, sum);
        prev_vec = sum;
        prev = vgetq_lane_u16(sum, 7);
    }
    for (; i < width; ++i)
    {
        line[i] = (uint16_t)(line[i] + prev);
        prev = line[i];
    }
#else
    (void)line;
    (void)width;
#endif
}

void TIFFPredictMedianDiff16_NEON(const uint16_t *curr, const uint16_t *prev,
                                  uint16_t *dst, size_t width)
{
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    if (width == 0)
        return;
    dst[0] = (uint16_t)(curr[0] - prev[0]);
    if (width == 1)
        return;
    size_t i = 1;
    size_t aligned_end = 1 + ((width - 1) & ~7U);
    for (; i < aligned_end; i += 8)
    {
        uint16_t a_arr[8];
        uint16_t b_arr[8];
        uint16_t g_arr[8];
        for (int j = 0; j < 8; j++)
        {
            a_arr[j] = curr[i + j - 1];
            b_arr[j] = prev[i + j];
            g_arr[j] = (uint16_t)(a_arr[j] + b_arr[j] - prev[i + j - 1]);
        }
        uint16x8_t A = vld1q_u16(a_arr);
        uint16x8_t B = vld1q_u16(b_arr);
        uint16x8_t Guess = vld1q_u16(g_arr);
        uint16x8_t med = vmaxq_u16(vminq_u16(A, B),
                                   vminq_u16(vmaxq_u16(A, B), Guess));
        uint16x8_t cur = vld1q_u16(curr + i);
        uint16x8_t diff = vsubq_u16(cur, med);
        vst1q_u16(dst + i, diff);
    }
    for (; i < width; ++i)
    {
        uint16_t A = curr[i - 1];
        uint16_t B = prev[i];
        uint16_t C = prev[i - 1];
        uint16_t guess = (uint16_t)(A + B - C);
        uint16_t minAB = A < B ? A : B;
        uint16_t maxAB = A > B ? A : B;
        uint16_t med = guess < minAB ? minAB : (guess > maxAB ? maxAB : guess);
        dst[i] = (uint16_t)(curr[i] - med);
    }
#else
    (void)curr;
    (void)prev;
    (void)dst;
    (void)width;
#endif
}
