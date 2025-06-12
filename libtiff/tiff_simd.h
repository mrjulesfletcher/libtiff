#ifndef TIFF_SIMD_H
#define TIFF_SIMD_H

#include <stdint.h>
#if defined(HAVE_NEON) && defined(__ARM_NEON)
#include <arm_neon.h>
#endif
#if defined(HAVE_SSE41)
#include <smmintrin.h>
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
#if TIFF_SIMD_NEON || TIFF_SIMD_SSE41
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
#if defined(HAVE_SSE41)
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

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* TIFF_SIMD_H */
