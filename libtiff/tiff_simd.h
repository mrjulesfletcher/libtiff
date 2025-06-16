#ifndef TIFF_SIMD_H
#define TIFF_SIMD_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#if defined(HAVE_NEON) && defined(__ARM_NEON)
#include <arm_neon.h>
#endif
#if defined(HAVE_SSE2)
#include <emmintrin.h>
#endif
#if defined(HAVE_SSE41)
#include <smmintrin.h>
#endif
#if defined(HAVE_SSE42)
#include <nmmintrin.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(HAVE_NEON)
#define TIFF_SIMD_NEON 1
#else
#define TIFF_SIMD_NEON 0
#endif
#if defined(HAVE_SSE41)
#define TIFF_SIMD_SSE41 1
#else
#define TIFF_SIMD_SSE41 0
#endif
#if defined(HAVE_SSE42)
#define TIFF_SIMD_SSE42 1
#else
#define TIFF_SIMD_SSE42 0
#endif
#if defined(HAVE_SSE2)
#define TIFF_SIMD_SSE2 1
#else
#define TIFF_SIMD_SSE2 0
#endif
#if TIFF_SIMD_NEON || TIFF_SIMD_SSE41 || TIFF_SIMD_SSE42 || TIFF_SIMD_SSE2
#define TIFF_SIMD_ENABLED 1
#else
#define TIFF_SIMD_ENABLED 0
#endif

    typedef union
    {
        uint8_t v[16];
#if defined(HAVE_NEON) && defined(__ARM_NEON)
        uint8x16_t n;
#endif
#if defined(HAVE_SSE41) || defined(HAVE_SSE2)
        __m128i x;
#endif
    } tiff_v16u8;

    typedef struct
    {
        tiff_v16u8 (*loadu_u8)(const uint8_t *);
        void (*storeu_u8)(uint8_t *, tiff_v16u8);
        tiff_v16u8 (*add_u8)(tiff_v16u8, tiff_v16u8);
        tiff_v16u8 (*sub_u8)(tiff_v16u8, tiff_v16u8);
    } tiff_simd_funcs;

    extern tiff_simd_funcs tiff_simd;
    extern int tiff_use_neon;
    extern int tiff_use_sse41;
    extern int tiff_use_sse2;
    extern int tiff_use_sse42;
    int TIFFUseNEON(void);
    int TIFFUseSSE41(void);
    int TIFFUseSSE2(void);
    int TIFFUseSSE42(void);
    void TIFFSetUseNEON(int);
    void TIFFSetUseSSE41(int);
    void TIFFSetUseSSE2(int);
    void TIFFSetUseSSE42(int);

    static inline tiff_v16u8 tiff_loadu_u8(const uint8_t *p)
    {
        return tiff_simd.loadu_u8(p);
    }
    static inline void tiff_storeu_u8(uint8_t *p, tiff_v16u8 v)
    {
        tiff_simd.storeu_u8(p, v);
    }
    static inline tiff_v16u8 tiff_add_u8(tiff_v16u8 a, tiff_v16u8 b)
    {
        return tiff_simd.add_u8(a, b);
    }
    static inline tiff_v16u8 tiff_sub_u8(tiff_v16u8 a, tiff_v16u8 b)
    {
        return tiff_simd.sub_u8(a, b);
    }

    static inline void tiff_memmove_u8(uint8_t *dst, const uint8_t *src,
                                       size_t n)
    {
#if defined(HAVE_NEON) && defined(__ARM_NEON)
        if (tiff_use_neon)
        {
            if (dst == src || n == 0)
                return;
            if (dst < src)
            {
                while (((uintptr_t)dst & 15) && n)
                {
                    *dst++ = *src++;
                    n--;
                }
                for (; n >= 16; n -= 16, dst += 16, src += 16)
                {
                    __builtin_prefetch(src + 64);
                    vst1q_u8(dst, vld1q_u8(src));
                }
                while (n--)
                    *dst++ = *src++;
            }
            else
            {
                dst += n;
                src += n;
                while (((uintptr_t)dst & 15) && n)
                {
                    dst--;
                    src--;
                    *dst = *src;
                    n--;
                }
                for (; n >= 16; n -= 16)
                {
                    dst -= 16;
                    src -= 16;
                    __builtin_prefetch(src - 64);
                    vst1q_u8(dst, vld1q_u8(src));
                }
                while (n--)
                {
                    dst--;
                    src--;
                    *dst = *src;
                }
        }
        return;
    }
#endif
#if defined(HAVE_SSE41)
        if (tiff_use_sse41)
        {
            if (dst == src || n == 0)
                return;
            if (dst < src)
            {
                while (((uintptr_t)dst & 15) && n)
                {
                    *dst++ = *src++;
                    n--;
                }
                for (; n >= 16; n -= 16, dst += 16, src += 16)
                {
                    __m128i v = _mm_loadu_si128((const __m128i *)src);
                    __builtin_prefetch(src + 64);
                    _mm_storeu_si128((__m128i *)dst, v);
                }
                while (n--)
                    *dst++ = *src++;
            }
            else
            {
                dst += n;
                src += n;
                while (((uintptr_t)dst & 15) && n)
                {
                    dst--;
                    src--;
                    *dst = *src;
                    n--;
                }
                for (; n >= 16; n -= 16)
                {
                    dst -= 16;
                    src -= 16;
                    __m128i v = _mm_loadu_si128((const __m128i *)src);
                    __builtin_prefetch(src - 64);
                    _mm_storeu_si128((__m128i *)dst, v);
                }
                while (n--)
                {
                    dst--;
                    src--;
                    *dst = *src;
                }
            }
            return;
        }
#endif
        memmove(dst, src, n);
    }

    static inline void tiff_memcpy_u8(uint8_t *dst, const uint8_t *src,
                                      size_t n)
    {
#if defined(HAVE_NEON) && defined(__ARM_NEON)
        if (tiff_use_neon)
        {
            while (((uintptr_t)dst & 15) && n)
            {
                *dst++ = *src++;
                n--;
            }
            for (; n >= 16; n -= 16, dst += 16, src += 16)
            {
                vst1q_u8(dst, vld1q_u8(src));
            }
            while (n--)
                *dst++ = *src++;
            return;
        }
#endif
#if defined(HAVE_SSE41)
        if (tiff_use_sse41)
        {
            while (((uintptr_t)dst & 15) && n)
            {
                *dst++ = *src++;
                n--;
            }
            for (; n >= 16; n -= 16, dst += 16, src += 16)
            {
                __m128i v = _mm_loadu_si128((const __m128i *)src);
                _mm_storeu_si128((__m128i *)dst, v);
            }
            while (n--)
                *dst++ = *src++;
            return;
        }
#endif
        memcpy(dst, src, n);
    }

    static inline void tiff_memset_u8(uint8_t *dst, uint8_t value, size_t n)
    {
#if defined(HAVE_NEON) && defined(__ARM_NEON)
        if (tiff_use_neon)
        {
            if (n >= 16)
            {
                uint8x16_t v = vdupq_n_u8(value);
                while (((uintptr_t)dst & 15) && n)
                {
                    *dst++ = value;
                    n--;
                }
                for (; n >= 16; n -= 16, dst += 16)
                {
                    vst1q_u8(dst, v);
                }
            }
            while (n--)
                *dst++ = value;
            return;
        }
#endif
#if defined(HAVE_SSE41)
        if (tiff_use_sse41)
        {
            if (n >= 16)
            {
                __m128i v = _mm_set1_epi8((char)value);
                while (((uintptr_t)dst & 15) && n)
                {
                    *dst++ = value;
                    n--;
                }
                for (; n >= 16; n -= 16, dst += 16)
                {
                    _mm_storeu_si128((__m128i *)dst, v);
                }
            }
            while (n--)
                *dst++ = value;
            return;
        }
#endif
        memset(dst, value, n);
    }

    uint32_t tiff_crc32(uint32_t crc, const uint8_t *buf, size_t len);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* TIFF_SIMD_H */
