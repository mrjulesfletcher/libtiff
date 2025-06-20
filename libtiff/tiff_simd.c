#include "tiff_simd.h"
#include <string.h>
/*
 * Runtime SIMD feature detection helpers.  When compiled on Linux the code
 * attempts to use getauxval(AT_HWCAP) first.  If that function is not
 * available, PR_GET_AUXV or the contents of /proc/cpuinfo are consulted.
 */
#if defined(__linux__)
#if defined(__has_include)
#if __has_include(<sys/auxv.h>)
#include <sys/auxv.h>
#define TIFF_HAVE_GETAUXVAL 1
#endif
#endif
#include <stdio.h>
#include <sys/prctl.h>
#endif
#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) ||             \
    defined(_M_IX86)
#include <cpuid.h>
#endif

tiff_simd_funcs tiff_simd;
int tiff_use_neon = 0;
int tiff_use_sse41 = 0;
int tiff_use_sse2 = 0;
int tiff_use_sse42 = 0;
int tiff_use_aes = 0;
int tiff_use_pmull = 0;
#ifdef ZIP_SUPPORT
#include <zlib.h>
#endif

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
    __builtin_prefetch(ptr + 64);
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

/*
 * detect_neon() checks for NEON availability at runtime on Linux.
 * 1. If getauxval() is present, AT_HWCAP is inspected for NEON bits.
 * 2. If getauxval() is unavailable, PR_GET_AUXV is tried.
 * 3. As a last resort /proc/cpuinfo is scanned for "neon" or "asimd".
 */
static int detect_neon(void)
{
#if defined(HAVE_NEON) && defined(__linux__)
    unsigned long hwcap = 0;
#ifdef TIFF_HAVE_GETAUXVAL
#ifdef __aarch64__
#ifdef HWCAP_ASIMD
    hwcap = getauxval(AT_HWCAP);
    if (hwcap & HWCAP_ASIMD)
        return 1;
#endif
#elif defined(__arm__)
#ifdef HWCAP_NEON
    hwcap = getauxval(AT_HWCAP);
    if (hwcap & HWCAP_NEON)
        return 1;
#endif
#endif
#endif /* TIFF_HAVE_GETAUXVAL */
#ifdef PR_GET_AUXV
    if (hwcap == 0)
    {
        unsigned long auxv[32];
        long r = prctl(PR_GET_AUXV, auxv, sizeof(auxv), 0, 0);
        if (r >= 0)
        {
            for (size_t i = 0; i + 1 < sizeof(auxv) / sizeof(auxv[0]); i += 2)
            {
                if (auxv[i] == AT_HWCAP)
                {
                    hwcap = auxv[i + 1];
                    break;
                }
            }
        }
    }
#endif /* PR_GET_AUXV */
    if (hwcap == 0)
    {
        FILE *f = fopen("/proc/cpuinfo", "r");
        if (f)
        {
            char line[256];
            while (fgets(line, sizeof(line), f))
            {
                if (strstr(line, "neon") || strstr(line, "asimd"))
                {
                    fclose(f);
                    return 1;
                }
            }
            fclose(f);
        }
    }
#ifdef HWCAP_ASIMD
    if (hwcap & HWCAP_ASIMD)
        return 1;
#endif
#ifdef HWCAP_NEON
    if (hwcap & HWCAP_NEON)
        return 1;
#endif
#endif /* HAVE_NEON && __linux__ */
    return 0;
}

static int detect_sse41(void)
{
#if defined(HAVE_SSE41) && (defined(__x86_64__) || defined(__i386__) || \
                            defined(_M_X64) || defined(_M_IX86))
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx))
        return (ecx & bit_SSE4_1) != 0;
#endif
    return 0;
}

static int detect_sse2(void)
{
#if defined(HAVE_SSE2) && (defined(__x86_64__) || defined(__i386__) || \
                            defined(_M_X64) || defined(_M_IX86))
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx))
        return (edx & bit_SSE2) != 0;
#endif
    return 0;
}

static int detect_sse42(void)
{
#if defined(HAVE_SSE42) && (defined(__x86_64__) || defined(__i386__) || \
                            defined(_M_X64) || defined(_M_IX86))
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx))
        return (ecx & bit_SSE4_2) != 0;
#endif
    return 0;
}

static int detect_aes(void)
{
#if defined(HAVE_HW_AES)
#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx))
        return (ecx & bit_AES) != 0;
    return 0;
#elif defined(__aarch64__) || defined(__arm__)
#if defined(__linux__)
    unsigned long hwcap2 = 0;
#ifdef TIFF_HAVE_GETAUXVAL
#ifdef AT_HWCAP2
    hwcap2 = getauxval(AT_HWCAP2);
#endif
#endif
#ifdef PR_GET_AUXV
    if (hwcap2 == 0)
    {
        unsigned long auxv[64];
        long r = prctl(PR_GET_AUXV, auxv, sizeof(auxv), 0, 0);
        if (r >= 0)
        {
            for (size_t i = 0; i + 1 < sizeof(auxv) / sizeof(auxv[0]); i += 2)
            {
                if (auxv[i] == AT_HWCAP2)
                {
                    hwcap2 = auxv[i + 1];
                    break;
                }
            }
        }
    }
#endif
#ifdef HWCAP2_AES
    if (hwcap2 & HWCAP2_AES)
        return 1;
#endif
#endif /* __linux__ */
#ifdef __ARM_FEATURE_CRYPTO
    return 1;
#endif
    return 0;
#else
    return 0;
#endif
#else
    return 0;
#endif
}

static int detect_pmull(void)
{
#if defined(HAVE_PMULL)
#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) ||                \
    defined(_M_IX86)
    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx))
        return (ecx & bit_PCLMUL) != 0;
    return 0;
#elif defined(__aarch64__) || defined(__arm__)
#if defined(__linux__)
    unsigned long hwcap2 = 0;
#ifdef TIFF_HAVE_GETAUXVAL
#ifdef AT_HWCAP2
    hwcap2 = getauxval(AT_HWCAP2);
#endif
#endif
#ifdef PR_GET_AUXV
    if (hwcap2 == 0)
    {
        unsigned long auxv[64];
        long r = prctl(PR_GET_AUXV, auxv, sizeof(auxv), 0, 0);
        if (r >= 0)
        {
            for (size_t i = 0; i + 1 < sizeof(auxv) / sizeof(auxv[0]); i += 2)
            {
                if (auxv[i] == AT_HWCAP2)
                {
                    hwcap2 = auxv[i + 1];
                    break;
                }
            }
        }
    }
#endif
#ifdef HWCAP2_PMULL
    if (hwcap2 & HWCAP2_PMULL)
        return 1;
#endif
#endif /* __linux__ */
#ifdef __ARM_FEATURE_CRYPTO
    return 1;
#endif
    return 0;
#else
    return 0;
#endif
#else
    return 0;
#endif
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
        tiff_use_neon = 1;
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
        tiff_use_sse41 = 1;
        tiff_simd.loadu_u8 = loadu_u8_sse41;
        tiff_simd.storeu_u8 = storeu_u8_sse41;
        tiff_simd.add_u8 = add_u8_sse41;
        tiff_simd.sub_u8 = sub_u8_sse41;
    }
    else
#endif
#if defined(HAVE_SSE2)
    if (detect_sse2())
    {
        tiff_use_sse2 = 1;
    }
#endif
#if defined(HAVE_SSE42)
    if (detect_sse42())
    {
        tiff_use_sse42 = 1;
    }
#endif
#if defined(HAVE_HW_AES)
    if (detect_aes())
    {
        tiff_use_aes = 1;
    }
#endif
#if defined(HAVE_PMULL)
    if (detect_pmull())
    {
        tiff_use_pmull = 1;
    }
#endif
}

int TIFFUseNEON(void) { return tiff_use_neon; }

int TIFFUseSSE41(void) { return tiff_use_sse41; }

int TIFFUseSSE2(void) { return tiff_use_sse2; }

int TIFFUseSSE42(void) { return tiff_use_sse42; }

void TIFFSetUseNEON(int enable) { tiff_use_neon = enable; }

void TIFFSetUseSSE41(int enable) { tiff_use_sse41 = enable; }

void TIFFSetUseSSE2(int enable) { tiff_use_sse2 = enable; }

void TIFFSetUseSSE42(int enable) { tiff_use_sse42 = enable; }

int TIFFUseAES(void) { return tiff_use_aes; }

void TIFFSetUseAES(int enable) { tiff_use_aes = enable; }

int TIFFUsePMULL(void) { return tiff_use_pmull; }

void TIFFSetUsePMULL(int enable) { tiff_use_pmull = enable; }

#if defined(HAVE_ARM_CRC32) && defined(__ARM_FEATURE_CRC32)
static uint32_t crc32_neon(uint32_t crc, const uint8_t *p, size_t len)
{
    while (len >= 8)
    {
        uint64_t v;
        memcpy(&v, p, 8);
        crc = __crc32d(crc, v);
        p += 8;
        len -= 8;
    }
    if (len >= 4)
    {
        uint32_t v;
        memcpy(&v, p, 4);
        crc = __crc32w(crc, v);
        p += 4;
        len -= 4;
    }
    if (len >= 2)
    {
        uint16_t v;
        memcpy(&v, p, 2);
        crc = __crc32h(crc, v);
        p += 2;
        len -= 2;
    }
    if (len)
        crc = __crc32b(crc, *p);
    return crc;
}
#endif

#if defined(HAVE_SSE42)
static uint32_t crc32_sse42(uint32_t crc, const uint8_t *p, size_t len)
{
    while (len >= 8)
    {
        uint64_t v;
        memcpy(&v, p, 8);
        crc = (uint32_t)_mm_crc32_u64(crc, v);
        p += 8;
        len -= 8;
    }
    if (len >= 4)
    {
        uint32_t v;
        memcpy(&v, p, 4);
        crc = _mm_crc32_u32(crc, v);
        p += 4;
        len -= 4;
    }
    if (len >= 2)
    {
        uint16_t v;
        memcpy(&v, p, 2);
        crc = _mm_crc32_u16(crc, v);
        p += 2;
        len -= 2;
    }
    if (len)
        crc = _mm_crc32_u8(crc, *p);
    return crc;
}
#endif

#ifndef ZIP_SUPPORT
static uint32_t crc32_generic(uint32_t crc, const uint8_t *p, size_t len)
{
    crc = ~crc;
    while (len--)
    {
        crc ^= *p++;
        for (int i = 0; i < 8; i++)
            crc = (crc >> 1) ^ (0xEDB88320U & (-(int)(crc & 1)));
    }
    return ~crc;
}
#endif

uint32_t tiff_crc32(uint32_t crc, const uint8_t *buf, size_t len)
{
#if defined(HAVE_ARM_CRC32) && defined(__ARM_FEATURE_CRC32)
    if (tiff_use_neon)
        return crc32_neon(crc, buf, len);
#endif
#if defined(HAVE_SSE42)
    if (tiff_use_sse42)
        return crc32_sse42(crc, buf, len);
#endif
#ifdef ZIP_SUPPORT
    return (uint32_t)crc32(crc, buf, (uInt)len);
#else
    return crc32_generic(crc, buf, len);
#endif
}

void tiff_aes_whiten(uint8_t *buf, size_t len)
{
#if TIFF_SIMD_AES
    if (!tiff_use_aes)
        return;
#if defined(__ARM_FEATURE_CRYPTO)
    uint8x16_t zero = vdupq_n_u8(0);
    while (len >= 16)
    {
        uint8x16_t b = vld1q_u8(buf);
        b = vaeseq_u8(b, zero);
        b = vaesmcq_u8(b);
        b = vaeseq_u8(b, zero);
        b = vaesmcq_u8(b);
        vst1q_u8(buf, b);
        buf += 16;
        len -= 16;
    }
    if (len)
    {
        uint8_t tmp[16] = {0};
        memcpy(tmp, buf, len);
        uint8x16_t b = vld1q_u8(tmp);
        b = vaeseq_u8(b, zero);
        b = vaesmcq_u8(b);
        b = vaeseq_u8(b, zero);
        b = vaesmcq_u8(b);
        vst1q_u8(tmp, b);
        memcpy(buf, tmp, len);
    }
#elif defined(__AES__)
    const __m128i zero = _mm_setzero_si128();
    while (len >= 16)
    {
        __m128i b = _mm_loadu_si128((const __m128i *)buf);
        b = _mm_aesenc_si128(b, zero);
        b = _mm_aesenc_si128(b, zero);
        _mm_storeu_si128((__m128i *)buf, b);
        buf += 16;
        len -= 16;
    }
    if (len)
    {
        uint8_t tmp[16] = {0};
        memcpy(tmp, buf, len);
        __m128i b = _mm_loadu_si128((const __m128i *)tmp);
        b = _mm_aesenc_si128(b, zero);
        b = _mm_aesenc_si128(b, zero);
        _mm_storeu_si128((__m128i *)tmp, b);
        memcpy(buf, tmp, len);
    }
#else
    (void)buf;
    (void)len;
#endif
#else
    (void)buf;
    (void)len;
#endif
}

void tiff_aes_unwhiten(uint8_t *buf, size_t len)
{
#if TIFF_SIMD_AES
    if (!tiff_use_aes)
        return;
#if defined(__ARM_FEATURE_CRYPTO)
    uint8x16_t zero = vdupq_n_u8(0);
    while (len >= 16)
    {
        uint8x16_t b = vld1q_u8(buf);
        b = vaesdq_u8(b, zero);
        b = vaesimcq_u8(b);
        b = vaesdq_u8(b, zero);
        b = vaesimcq_u8(b);
        vst1q_u8(buf, b);
        buf += 16;
        len -= 16;
    }
    if (len)
    {
        uint8_t tmp[16] = {0};
        memcpy(tmp, buf, len);
        uint8x16_t b = vld1q_u8(tmp);
        b = vaesdq_u8(b, zero);
        b = vaesimcq_u8(b);
        b = vaesdq_u8(b, zero);
        b = vaesimcq_u8(b);
        vst1q_u8(tmp, b);
        memcpy(buf, tmp, len);
    }
#elif defined(__AES__)
    const __m128i zero = _mm_setzero_si128();
    while (len >= 16)
    {
        __m128i b = _mm_loadu_si128((const __m128i *)buf);
        b = _mm_aesdec_si128(b, zero);
        b = _mm_aesdec_si128(b, zero);
        _mm_storeu_si128((__m128i *)buf, b);
        buf += 16;
        len -= 16;
    }
    if (len)
    {
        uint8_t tmp[16] = {0};
        memcpy(tmp, buf, len);
        __m128i b = _mm_loadu_si128((const __m128i *)tmp);
        b = _mm_aesdec_si128(b, zero);
        b = _mm_aesdec_si128(b, zero);
        _mm_storeu_si128((__m128i *)tmp, b);
        memcpy(buf, tmp, len);
    }
#else
    (void)buf;
    (void)len;
#endif
#else
    (void)buf;
    (void)len;
#endif
}

/* PMULL-based 64-bit Galois hash */

#define PMULL_POLY 0x1D872B41A9C8D9FDULL

static uint64_t pmull_hash_generic(uint64_t h, const uint8_t *p, size_t len)
{
    while (len >= 8)
    {
        uint64_t v;
        memcpy(&v, p, 8);
        __uint128_t prod = (__uint128_t)(h ^ v) * PMULL_POLY;
        h = (uint64_t)(prod ^ (prod >> 64));
        p += 8;
        len -= 8;
    }
    if (len)
    {
        uint64_t v = 0;
        memcpy(&v, p, len);
        __uint128_t prod = (__uint128_t)(h ^ v) * PMULL_POLY;
        h = (uint64_t)(prod ^ (prod >> 64));
    }
    return h;
}

#if defined(HAVE_PMULL) && defined(__ARM_FEATURE_CRYPTO)
static uint64_t pmull_hash_neon(uint64_t h, const uint8_t *p, size_t len)
{
    const poly64_t poly = (poly64_t)PMULL_POLY;
    poly64_t acc = (poly64_t)h;
    while (len >= 8)
    {
        uint64_t v;
        memcpy(&v, p, 8);
        acc = veor_p64(acc, *((poly64_t *)&v));
        poly128_t prod = vmull_p64(acc, poly);
        acc = veor_p64(vget_low_p64(prod), vget_high_p64(prod));
        p += 8;
        len -= 8;
    }
    if (len)
    {
        uint64_t v = 0;
        memcpy(&v, p, len);
        acc = veor_p64(acc, *((poly64_t *)&v));
        poly128_t prod = vmull_p64(acc, poly);
        acc = veor_p64(vget_low_p64(prod), vget_high_p64(prod));
    }
    return (uint64_t)acc;
}
#endif

#if defined(HAVE_PMULL) && defined(__PCLMUL__)
static uint64_t pmull_hash_pclmul(uint64_t h, const uint8_t *p, size_t len)
{
    const __m128i poly = _mm_set_epi64x(PMULL_POLY, 0);
    __m128i acc = _mm_set_epi64x(0, h);
    while (len >= 8)
    {
        uint64_t v;
        memcpy(&v, p, 8);
        __m128i blk = _mm_set_epi64x(0, v);
        acc = _mm_xor_si128(acc, blk);
        __m128i prod = _mm_clmulepi64_si128(acc, poly, 0x00);
        uint64_t lo = (uint64_t)_mm_cvtsi128_si64(prod);
        uint64_t hi = (uint64_t)_mm_cvtsi128_si64(_mm_srli_si128(prod, 8));
        acc = _mm_set_epi64x(0, lo ^ hi);
        p += 8;
        len -= 8;
    }
    if (len)
    {
        uint64_t v = 0;
        memcpy(&v, p, len);
        __m128i blk = _mm_set_epi64x(0, v);
        acc = _mm_xor_si128(acc, blk);
        __m128i prod = _mm_clmulepi64_si128(acc, poly, 0x00);
        uint64_t lo = (uint64_t)_mm_cvtsi128_si64(prod);
        uint64_t hi = (uint64_t)_mm_cvtsi128_si64(_mm_srli_si128(prod, 8));
        acc = _mm_set_epi64x(0, lo ^ hi);
    }
    return (uint64_t)_mm_cvtsi128_si64(acc);
}
#endif

uint64_t tiff_pmull_hash(uint64_t h, const uint8_t *buf, size_t len)
{
#if defined(HAVE_PMULL) && defined(__ARM_FEATURE_CRYPTO)
    if (tiff_use_pmull)
        return pmull_hash_neon(h, buf, len);
#endif
#if defined(HAVE_PMULL) && defined(__PCLMUL__)
    if (tiff_use_pmull)
        return pmull_hash_pclmul(h, buf, len);
#endif
    return pmull_hash_generic(h, buf, len);
}
