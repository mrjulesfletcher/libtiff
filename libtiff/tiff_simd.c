#include "tiff_simd.h"
#include <string.h>
#if defined(__linux__)
#include <sys/auxv.h>
#endif
#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) ||             \
    defined(_M_IX86)
#include <cpuid.h>
#endif

tiff_simd_funcs tiff_simd;

/* Scalar implementations */
static tiff_v16u8 loadu_u8_scalar(const uint8_t *ptr)
{
    tiff_v16u8 r;
    memcpy(r.v, ptr, 16);
    return r;
}

static void storeu_u8_scalar(uint8_t *ptr, tiff_v16u8 v)
{
    memcpy(ptr, v.v, 16);
}

static tiff_v16u8 add_u8_scalar(tiff_v16u8 a, tiff_v16u8 b)
{
    tiff_v16u8 r;
    for (int i = 0; i < 16; i++)
        r.v[i] = (uint8_t)(a.v[i] + b.v[i]);
    return r;
}

static tiff_v16u8 sub_u8_scalar(tiff_v16u8 a, tiff_v16u8 b)
{
    tiff_v16u8 r;
    for (int i = 0; i < 16; i++)
        r.v[i] = (uint8_t)(a.v[i] - b.v[i]);
    return r;
}

#if defined(HAVE_NEON) && defined(__ARM_NEON)
#include <arm_neon.h>
static tiff_v16u8 loadu_u8_neon(const uint8_t *ptr)
{
    tiff_v16u8 r;
    r.n = vld1q_u8(ptr);
    return r;
}
static void storeu_u8_neon(uint8_t *ptr, tiff_v16u8 v) { vst1q_u8(ptr, v.n); }
static tiff_v16u8 add_u8_neon(tiff_v16u8 a, tiff_v16u8 b)
{
    tiff_v16u8 r;
    r.n = vaddq_u8(a.n, b.n);
    return r;
}
static tiff_v16u8 sub_u8_neon(tiff_v16u8 a, tiff_v16u8 b)
{
    tiff_v16u8 r;
    r.n = vsubq_u8(a.n, b.n);
    return r;
}
#endif

#if defined(HAVE_SSE41)
#include <smmintrin.h>
static tiff_v16u8 loadu_u8_sse41(const uint8_t *ptr)
{
    tiff_v16u8 r;
    r.x = _mm_loadu_si128((const __m128i *)ptr);
    return r;
}
static void storeu_u8_sse41(uint8_t *ptr, tiff_v16u8 v)
{
    _mm_storeu_si128((__m128i *)ptr, v.x);
}
static tiff_v16u8 add_u8_sse41(tiff_v16u8 a, tiff_v16u8 b)
{
    tiff_v16u8 r;
    r.x = _mm_add_epi8(a.x, b.x);
    return r;
}
static tiff_v16u8 sub_u8_sse41(tiff_v16u8 a, tiff_v16u8 b)
{
    tiff_v16u8 r;
    r.x = _mm_sub_epi8(a.x, b.x);
    return r;
}
#endif

static int detect_neon(void)
{
#if defined(HAVE_NEON) && defined(__linux__)
#ifdef __aarch64__
#ifdef HWCAP_ASIMD
    return (getauxval(AT_HWCAP) & HWCAP_ASIMD) != 0;
#endif
#elif defined(__arm__)
#ifdef HWCAP_NEON
    return (getauxval(AT_HWCAP) & HWCAP_NEON) != 0;
#endif
#endif
#endif
    return 0;
}

static int detect_sse41(void)
{
#if defined(HAVE_SSE41) && (defined(__x86_64__) || defined(__i386__) ||        \
                            defined(_M_X64) || defined(_M_IX86))
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx))
        return (ecx & bit_SSE4_1) != 0;
#endif
    return 0;
}

void TIFFInitSIMD(void)
{
    tiff_simd.loadu_u8 = loadu_u8_scalar;
    tiff_simd.storeu_u8 = storeu_u8_scalar;
    tiff_simd.add_u8 = add_u8_scalar;
    tiff_simd.sub_u8 = sub_u8_scalar;
#if defined(HAVE_NEON)
    if (detect_neon())
    {
        tiff_simd.loadu_u8 = loadu_u8_neon;
        tiff_simd.storeu_u8 = storeu_u8_neon;
        tiff_simd.add_u8 = add_u8_neon;
        tiff_simd.sub_u8 = sub_u8_neon;
        return;
    }
#endif
#if defined(HAVE_SSE41)
    if (detect_sse41())
    {
        tiff_simd.loadu_u8 = loadu_u8_sse41;
        tiff_simd.storeu_u8 = storeu_u8_sse41;
        tiff_simd.add_u8 = add_u8_sse41;
        tiff_simd.sub_u8 = sub_u8_sse41;
    }
#endif
}
