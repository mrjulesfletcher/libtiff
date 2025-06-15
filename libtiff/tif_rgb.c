#include "rgb_neon.h"
#include "tiff_simd.h"
#include <string.h>
#if defined(HAVE_NEON) && defined(__ARM_NEON)
#include <arm_neon.h>
#endif

static void pack_rgb24_scalar(const uint8_t *src, uint32_t *dst, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        dst[i] = ((uint32_t)src[i * 3 + 0]) | ((uint32_t)src[i * 3 + 1] << 8) |
                 ((uint32_t)src[i * 3 + 2] << 16) | 0xFF000000U;
    }
}

static void pack_rgba32_scalar(const uint8_t *src, uint32_t *dst, size_t count)
{
    memcpy(dst, src, count * 4);
}

static void pack_rgb48_scalar(const uint16_t *src, uint32_t *dst, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        dst[i] = ((uint32_t)(src[i * 3 + 0] >> 8)) |
                 ((uint32_t)(src[i * 3 + 1] >> 8) << 8) |
                 ((uint32_t)(src[i * 3 + 2] >> 8) << 16) | 0xFF000000U;
    }
}

static void pack_rgba64_scalar(const uint16_t *src, uint32_t *dst, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        dst[i] = ((uint32_t)(src[i * 4 + 0] >> 8)) |
                 ((uint32_t)(src[i * 4 + 1] >> 8) << 8) |
                 ((uint32_t)(src[i * 4 + 2] >> 8) << 16) |
                 ((uint32_t)(src[i * 4 + 3] >> 8) << 24);
    }
}

#if defined(HAVE_NEON) && defined(__ARM_NEON)
static void pack_rgb24_neon(const uint8_t *src, uint32_t *dst, size_t count)
{
    size_t i = 0;
    uint8x16_t alpha = vdupq_n_u8(255);
    for (; i + 16 <= count; i += 16)
    {
        __builtin_prefetch(src + i * 3 + 64);
        uint8x16x3_t v = vld3q_u8(src + i * 3);
        uint8x16x4_t outv = {v.val[0], v.val[1], v.val[2], alpha};
        vst4q_u8((uint8_t *)(dst + i), outv);
    }
    if (i < count)
        pack_rgb24_scalar(src + i * 3, dst + i, count - i);
}

static void pack_rgba32_neon(const uint8_t *src, uint32_t *dst, size_t count)
{
    size_t i = 0;
    for (; i + 16 <= count; i += 16)
    {
        __builtin_prefetch(src + i * 4 + 64);
        uint8x16x4_t v = vld4q_u8(src + i * 4);
        vst4q_u8((uint8_t *)(dst + i), v);
    }
    if (i < count)
        pack_rgba32_scalar(src + i * 4, dst + i, count - i);
}

static void pack_rgb48_neon(const uint16_t *src, uint32_t *dst, size_t count)
{
    size_t i = 0;
    uint8x8_t alpha = vdup_n_u8(255);
    for (; i + 8 <= count; i += 8)
    {
        __builtin_prefetch(src + i * 3 + 24);
        uint16x8x3_t v = vld3q_u16(src + i * 3);
        uint8x8_t r = vshrn_n_u16(v.val[0], 8);
        uint8x8_t g = vshrn_n_u16(v.val[1], 8);
        uint8x8_t b = vshrn_n_u16(v.val[2], 8);
        uint8x8x4_t outv = {r, g, b, alpha};
        vst4_u8((uint8_t *)(dst + i), outv);
    }
    if (i < count)
        pack_rgb48_scalar(src + i * 3, dst + i, count - i);
}

static void pack_rgba64_neon(const uint16_t *src, uint32_t *dst, size_t count)
{
    size_t i = 0;
    for (; i + 8 <= count; i += 8)
    {
        __builtin_prefetch(src + i * 4 + 32);
        uint16x8x4_t v = vld4q_u16(src + i * 4);
        uint8x8_t r = vshrn_n_u16(v.val[0], 8);
        uint8x8_t g = vshrn_n_u16(v.val[1], 8);
        uint8x8_t b = vshrn_n_u16(v.val[2], 8);
        uint8x8_t a = vshrn_n_u16(v.val[3], 8);
        uint8x8x4_t outv = {r, g, b, a};
        vst4_u8((uint8_t *)(dst + i), outv);
    }
    if (i < count)
        pack_rgba64_scalar(src + i * 4, dst + i, count - i);
}
#endif

void TIFFPackRGB24(const uint8_t *src, uint32_t *dst, size_t count)
{
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    if (tiff_use_neon)
        pack_rgb24_neon(src, dst, count);
    else
#endif
        pack_rgb24_scalar(src, dst, count);
}

void TIFFPackRGBA32(const uint8_t *src, uint32_t *dst, size_t count)
{
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    if (tiff_use_neon)
        pack_rgba32_neon(src, dst, count);
    else
#endif
        pack_rgba32_scalar(src, dst, count);
}

void TIFFPackRGB48(const uint16_t *src, uint32_t *dst, size_t count)
{
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    if (tiff_use_neon)
        pack_rgb48_neon(src, dst, count);
    else
#endif
        pack_rgb48_scalar(src, dst, count);
}

void TIFFPackRGBA64(const uint16_t *src, uint32_t *dst, size_t count)
{
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    if (tiff_use_neon)
        pack_rgba64_neon(src, dst, count);
    else
#endif
        pack_rgba64_scalar(src, dst, count);
}
