#ifndef TIFF_SIMD_H
#define TIFF_SIMD_H

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(HAVE_NEON) && defined(__ARM_NEON)
#include <arm_neon.h>
#define TIFF_SIMD_NEON 1
#define TIFF_SIMD_SSE41 0
#define TIFF_SIMD_ENABLED 1
typedef uint8x16_t tiff_v16u8;
static inline tiff_v16u8 tiff_loadu_u8(const uint8_t *ptr)
{
    return vld1q_u8(ptr);
}
static inline void tiff_storeu_u8(uint8_t *ptr, tiff_v16u8 v)
{
    vst1q_u8(ptr, v);
}
static inline tiff_v16u8 tiff_add_u8(tiff_v16u8 a, tiff_v16u8 b)
{
    return vaddq_u8(a, b);
}
static inline tiff_v16u8 tiff_sub_u8(tiff_v16u8 a, tiff_v16u8 b)
{
    return vsubq_u8(a, b);
}

#elif defined(HAVE_SSE41) && (defined(__SSE4_1__) || defined(_MSC_VER))
#include <smmintrin.h>
#define TIFF_SIMD_NEON 0
#define TIFF_SIMD_SSE41 1
#define TIFF_SIMD_ENABLED 1
typedef __m128i tiff_v16u8;
static inline tiff_v16u8 tiff_loadu_u8(const uint8_t *ptr)
{
    return _mm_loadu_si128((const __m128i *)ptr);
}
static inline void tiff_storeu_u8(uint8_t *ptr, tiff_v16u8 v)
{
    _mm_storeu_si128((__m128i *)ptr, v);
}
static inline tiff_v16u8 tiff_add_u8(tiff_v16u8 a, tiff_v16u8 b)
{
    return _mm_add_epi8(a, b);
}
static inline tiff_v16u8 tiff_sub_u8(tiff_v16u8 a, tiff_v16u8 b)
{
    return _mm_sub_epi8(a, b);
}

#else
#define TIFF_SIMD_NEON 0
#define TIFF_SIMD_SSE41 0
#define TIFF_SIMD_ENABLED 0
typedef struct
{
    uint8_t v[16];
} tiff_v16u8;
static inline tiff_v16u8 tiff_loadu_u8(const uint8_t *ptr)
{
    tiff_v16u8 r;
    memcpy(r.v, ptr, 16);
    return r;
}
static inline void tiff_storeu_u8(uint8_t *ptr, tiff_v16u8 v)
{
    memcpy(ptr, v.v, 16);
}
static inline tiff_v16u8 tiff_add_u8(tiff_v16u8 a, tiff_v16u8 b)
{
    tiff_v16u8 r;
    for (int i = 0; i < 16; i++)
        r.v[i] = (uint8_t)(a.v[i] + b.v[i]);
    return r;
}
static inline tiff_v16u8 tiff_sub_u8(tiff_v16u8 a, tiff_v16u8 b)
{
    tiff_v16u8 r;
    for (int i = 0; i < 16; i++)
        r.v[i] = (uint8_t)(a.v[i] - b.v[i]);
    return r;
}
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* TIFF_SIMD_H */
