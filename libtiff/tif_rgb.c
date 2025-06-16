#include "rgb_neon.h"
#include "tiff_simd.h"
#include <string.h>
#if defined(HAVE_NEON) && defined(__ARM_NEON)
#include <arm_neon.h>
#endif
#if defined(HAVE_SSE41)
#include <smmintrin.h>


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

static void pack_rgb24_sse41(const uint8_t *src, uint32_t *dst, size_t count)
{
    const __m128i alpha = _mm_set1_epi8((char)0xFF);
    const __m128i rmask0 =
        _mm_setr_epi8(0, 3, 6, 9, 12, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const __m128i rmask1 =
        _mm_setr_epi8(2, 5, 8, 11, 14, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const __m128i rmask2 =
        _mm_setr_epi8(1, 4, 7, 10, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const __m128i gmask0 =
        _mm_setr_epi8(1, 4, 7, 10, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const __m128i gmask1 =
        _mm_setr_epi8(0, 3, 6, 9, 12, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const __m128i gmask2 =
        _mm_setr_epi8(2, 5, 8, 11, 14, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const __m128i bmask0 =
        _mm_setr_epi8(2, 5, 8, 11, 14, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const __m128i bmask1 =
        _mm_setr_epi8(1, 4, 7, 10, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const __m128i bmask2 =
        _mm_setr_epi8(0, 3, 6, 9, 12, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    size_t i = 0;
    for (; i + 16 <= count; i += 16)
    {
        __builtin_prefetch(src + i * 3 + 64);
        __m128i t0 = _mm_loadu_si128((const __m128i *)(src + i * 3));
        __m128i t1 = _mm_loadu_si128((const __m128i *)(src + i * 3 + 16));
        __m128i t2 = _mm_loadu_si128((const __m128i *)(src + i * 3 + 32));
        __m128i r = _mm_or_si128(
            _mm_or_si128(_mm_shuffle_epi8(t0, rmask0),
                         _mm_slli_si128(_mm_shuffle_epi8(t1, rmask1), 6)),
            _mm_slli_si128(_mm_shuffle_epi8(t2, rmask2), 11));
        __m128i g = _mm_or_si128(
            _mm_or_si128(_mm_shuffle_epi8(t0, gmask0),
                         _mm_slli_si128(_mm_shuffle_epi8(t1, gmask1), 5)),
            _mm_slli_si128(_mm_shuffle_epi8(t2, gmask2), 11));
        __m128i b = _mm_or_si128(
            _mm_or_si128(_mm_shuffle_epi8(t0, bmask0),
                         _mm_slli_si128(_mm_shuffle_epi8(t1, bmask1), 5)),
            _mm_slli_si128(_mm_shuffle_epi8(t2, bmask2), 10));

        __m128i rg_lo = _mm_unpacklo_epi8(r, g);
        __m128i rg_hi = _mm_unpackhi_epi8(r, g);
        __m128i ba_lo = _mm_unpacklo_epi8(b, alpha);
        __m128i ba_hi = _mm_unpackhi_epi8(b, alpha);
        _mm_storeu_si128((__m128i *)(dst + i + 0),
                         _mm_unpacklo_epi16(rg_lo, ba_lo));
        _mm_storeu_si128((__m128i *)(dst + i + 4),
                         _mm_unpackhi_epi16(rg_lo, ba_lo));
        _mm_storeu_si128((__m128i *)(dst + i + 8),
                         _mm_unpacklo_epi16(rg_hi, ba_hi));
        _mm_storeu_si128((__m128i *)(dst + i + 12),
                         _mm_unpackhi_epi16(rg_hi, ba_hi));
    }
    if (i < count)
        pack_rgb24_scalar(src + i * 3, dst + i, count - i);
}

static void pack_rgba32_sse41(const uint8_t *src, uint32_t *dst, size_t count)
{
    const __m128i mask =
        _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
    size_t i = 0;
    for (; i + 16 <= count; i += 16)
    {
        __builtin_prefetch(src + i * 4 + 64);
        __m128i v0 = _mm_loadu_si128((const __m128i *)(src + i * 4));
        __m128i v1 = _mm_loadu_si128((const __m128i *)(src + i * 4 + 16));
        __m128i v2 = _mm_loadu_si128((const __m128i *)(src + i * 4 + 32));
        __m128i v3 = _mm_loadu_si128((const __m128i *)(src + i * 4 + 48));
        v0 = _mm_shuffle_epi8(v0, mask);
        v1 = _mm_shuffle_epi8(v1, mask);
        v2 = _mm_shuffle_epi8(v2, mask);
        v3 = _mm_shuffle_epi8(v3, mask);
        _mm_storeu_si128((__m128i *)(dst + i + 0), v0);
        _mm_storeu_si128((__m128i *)(dst + i + 4), v1);
        _mm_storeu_si128((__m128i *)(dst + i + 8), v2);
        _mm_storeu_si128((__m128i *)(dst + i + 12), v3);
    }
    if (i < count)
        pack_rgba32_scalar(src + i * 4, dst + i, count - i);
}

static void pack_rgb48_sse41(const uint16_t *src, uint32_t *dst, size_t count)
{
    const __m128i alpha = _mm_set1_epi8((char)0xFF);
    const __m128i rmask0 =
        _mm_setr_epi8(0, 1, 6, 7, 12, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const __m128i rmask1 =
        _mm_setr_epi8(2, 3, 8, 9, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const __m128i rmask2 =
        _mm_setr_epi8(4, 5, 10, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const __m128i gmask0 =
        _mm_setr_epi8(2, 3, 8, 9, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const __m128i gmask1 =
        _mm_setr_epi8(4, 5, 10, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const __m128i gmask2 =
        _mm_setr_epi8(0, 1, 6, 7, 12, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const __m128i bmask0 =
        _mm_setr_epi8(4, 5, 10, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const __m128i bmask1 =
        _mm_setr_epi8(0, 1, 6, 7, 12, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const __m128i bmask2 =
        _mm_setr_epi8(2, 3, 8, 9, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);

    const __m128i low8 =
        _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1);

    size_t i = 0;
    for (; i + 8 <= count; i += 8)
    {
        __builtin_prefetch(src + i * 3 + 24);
        const __m128i t0 = _mm_loadu_si128((const __m128i *)(src + i * 3));
        const __m128i t1 = _mm_loadu_si128((const __m128i *)(src + i * 3 + 8));
        const __m128i t2 = _mm_loadu_si128((const __m128i *)(src + i * 3 + 16));

        __m128i r = _mm_or_si128(
            _mm_or_si128(_mm_shuffle_epi8(t0, rmask0),
                         _mm_slli_si128(_mm_shuffle_epi8(t1, rmask1), 6)),
            _mm_slli_si128(_mm_shuffle_epi8(t2, rmask2), 12));
        __m128i g = _mm_or_si128(
            _mm_or_si128(_mm_shuffle_epi8(t0, gmask0),
                         _mm_slli_si128(_mm_shuffle_epi8(t1, gmask1), 4)),
            _mm_slli_si128(_mm_shuffle_epi8(t2, gmask2), 10));
        __m128i b = _mm_or_si128(
            _mm_or_si128(_mm_shuffle_epi8(t0, bmask0),
                         _mm_slli_si128(_mm_shuffle_epi8(t1, bmask1), 4)),
            _mm_slli_si128(_mm_shuffle_epi8(t2, bmask2), 10));

        r = _mm_srli_epi16(r, 8);
        g = _mm_srli_epi16(g, 8);
        b = _mm_srli_epi16(b, 8);
        __m128i r8 = _mm_shuffle_epi8(_mm_packus_epi16(r, r), low8);
        __m128i g8 = _mm_shuffle_epi8(_mm_packus_epi16(g, g), low8);
        __m128i b8 = _mm_shuffle_epi8(_mm_packus_epi16(b, b), low8);

        __m128i rg = _mm_unpacklo_epi8(r8, g8);
        __m128i ba = _mm_unpacklo_epi8(b8, alpha);
        _mm_storeu_si128((__m128i *)(dst + i + 0),
                         _mm_unpacklo_epi16(rg, ba));
        _mm_storeu_si128((__m128i *)(dst + i + 4),
                         _mm_unpackhi_epi16(rg, ba));
    }
    if (i < count)
        pack_rgb48_scalar(src + i * 3, dst + i, count - i);
}

static void pack_rgba64_sse41(const uint16_t *src, uint32_t *dst, size_t count)
{
    const __m128i rmask =
        _mm_setr_epi8(0, 1, 8, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const __m128i gmask =
        _mm_setr_epi8(2, 3, 10, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                      -1);
    const __m128i bmask =
        _mm_setr_epi8(4, 5, 12, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                      -1);
    const __m128i amask =
        _mm_setr_epi8(6, 7, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                      -1);
    const __m128i alpha_mask =
        _mm_setr_epi8(0, 1, 2, 3, 4, 5, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1);

    size_t i = 0;
    for (; i + 8 <= count; i += 8)
    {
        __builtin_prefetch(src + i * 4 + 32);
        __m128i t0 = _mm_loadu_si128((const __m128i *)(src + i * 4));
        __m128i t1 = _mm_loadu_si128((const __m128i *)(src + i * 4 + 8));
        __m128i t2 = _mm_loadu_si128((const __m128i *)(src + i * 4 + 16));
        __m128i t3 = _mm_loadu_si128((const __m128i *)(src + i * 4 + 24));

        __m128i r = _mm_or_si128(
            _mm_or_si128(_mm_shuffle_epi8(t0, rmask),
                         _mm_slli_si128(_mm_shuffle_epi8(t1, rmask), 4)),
            _mm_or_si128(_mm_slli_si128(_mm_shuffle_epi8(t2, rmask), 8),
                         _mm_slli_si128(_mm_shuffle_epi8(t3, rmask), 12)));
        __m128i g = _mm_or_si128(
            _mm_or_si128(_mm_shuffle_epi8(t0, gmask),
                         _mm_slli_si128(_mm_shuffle_epi8(t1, gmask), 4)),
            _mm_or_si128(_mm_slli_si128(_mm_shuffle_epi8(t2, gmask), 8),
                         _mm_slli_si128(_mm_shuffle_epi8(t3, gmask), 12)));
        __m128i b = _mm_or_si128(
            _mm_or_si128(_mm_shuffle_epi8(t0, bmask),
                         _mm_slli_si128(_mm_shuffle_epi8(t1, bmask), 4)),
            _mm_or_si128(_mm_slli_si128(_mm_shuffle_epi8(t2, bmask), 8),
                         _mm_slli_si128(_mm_shuffle_epi8(t3, bmask), 12)));
        __m128i a = _mm_or_si128(
            _mm_or_si128(_mm_shuffle_epi8(t0, amask),
                         _mm_slli_si128(_mm_shuffle_epi8(t1, amask), 4)),
            _mm_or_si128(_mm_slli_si128(_mm_shuffle_epi8(t2, amask), 8),
                         _mm_slli_si128(_mm_shuffle_epi8(t3, amask), 12)));

        r = _mm_srli_epi16(r, 8);
        g = _mm_srli_epi16(g, 8);
        b = _mm_srli_epi16(b, 8);
        a = _mm_srli_epi16(a, 8);
        __m128i r8 = _mm_shuffle_epi8(_mm_packus_epi16(r, r), alpha_mask);
        __m128i g8 = _mm_shuffle_epi8(_mm_packus_epi16(g, g), alpha_mask);
        __m128i b8 = _mm_shuffle_epi8(_mm_packus_epi16(b, b), alpha_mask);
        __m128i a8 = _mm_shuffle_epi8(_mm_packus_epi16(a, a), alpha_mask);

        __m128i rg = _mm_unpacklo_epi8(r8, g8);
        __m128i ba = _mm_unpacklo_epi8(b8, a8);
        _mm_storeu_si128((__m128i *)(dst + i + 0),
                         _mm_unpacklo_epi16(rg, ba));
        _mm_storeu_si128((__m128i *)(dst + i + 4),
                         _mm_unpackhi_epi16(rg, ba));
    }
    if (i < count)
        pack_rgba64_scalar(src + i * 4, dst + i, count - i);
}
#endif
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
#if defined(HAVE_SSE41)
    if (tiff_use_sse41)
        pack_rgb24_sse41(src, dst, count);
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
#if defined(HAVE_SSE41)
    if (tiff_use_sse41)
        pack_rgba32_sse41(src, dst, count);
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
#if defined(HAVE_SSE41)
    if (tiff_use_sse41)
        pack_rgb48_sse41(src, dst, count);
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
#if defined(HAVE_SSE41)
    if (tiff_use_sse41)
        pack_rgba64_sse41(src, dst, count);
    else
#endif
        pack_rgba64_scalar(src, dst, count);
}
