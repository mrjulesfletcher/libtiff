/*
 * Copyright (c) 1988-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

/*
 * TIFF Library.
 *
 * Predictor Tag Support (used by multiple codecs).
 */
#include "tif_predict.h"
#include "tiffiop.h"

#if defined(HAVE_SSE2)
#include <emmintrin.h>
#endif
#if defined(HAVE_SSE41)
#include <smmintrin.h>
#endif
#if defined(HAVE_NEON) && defined(__ARM_NEON)
#include <arm_neon.h>
#endif

static inline uint16_t add_sat_u16(uint16_t a, uint16_t b)
{
    uint32_t sum = (uint32_t)a + (uint32_t)b;
    if (sum > 65535U)
        sum = 65535U;
    return (uint16_t)sum;
}

#define PredictorState(tif) ((TIFFPredictorState *)(tif)->tif_data)

static int horAcc8(TIFF *tif, uint8_t *cp0, tmsize_t cc);
static int horAcc16(TIFF *tif, uint8_t *cp0, tmsize_t cc);
static int horAcc32(TIFF *tif, uint8_t *cp0, tmsize_t cc);
static int horAcc64(TIFF *tif, uint8_t *cp0, tmsize_t cc);
static int swabHorAcc16(TIFF *tif, uint8_t *cp0, tmsize_t cc);
static int swabHorAcc32(TIFF *tif, uint8_t *cp0, tmsize_t cc);
static int swabHorAcc64(TIFF *tif, uint8_t *cp0, tmsize_t cc);
static int horDiff8(TIFF *tif, uint8_t *cp0, tmsize_t cc);
static int horDiff16(TIFF *tif, uint8_t *cp0, tmsize_t cc);
static int horDiff32(TIFF *tif, uint8_t *cp0, tmsize_t cc);
static int horDiff64(TIFF *tif, uint8_t *cp0, tmsize_t cc);
static int swabHorDiff16(TIFF *tif, uint8_t *cp0, tmsize_t cc);
static int swabHorDiff32(TIFF *tif, uint8_t *cp0, tmsize_t cc);
static int swabHorDiff64(TIFF *tif, uint8_t *cp0, tmsize_t cc);
static int fpAcc(TIFF *tif, uint8_t *cp0, tmsize_t cc);
static int fpDiff(TIFF *tif, uint8_t *cp0, tmsize_t cc);
#if defined(__AVX2__)
static void interleave4_avx2(uint8_t *dst, const uint8_t *src, tmsize_t wc,
                             const int order[4]);
static void interleave8_avx2(uint8_t *dst, const uint8_t *src, tmsize_t wc,
                             const int oh[4], const int ol[4]);
#endif
#if defined(HAVE_NEON) && defined(__ARM_NEON)
static void interleave4_neon(uint8_t *dst, const uint8_t *src, tmsize_t wc,
                             const int order[4]);
static void interleave8_neon(uint8_t *dst, const uint8_t *src, tmsize_t wc,
                             const int oh[4], const int ol[4]);
#endif

#if defined(__AVX2__)
static void interleave4_avx2(uint8_t *dst, const uint8_t *src, tmsize_t wc,
                             const int order[4])
{
    tmsize_t i = 0;
    for (; i + 31 < wc; i += 32)
    {
        __builtin_prefetch(src + i + 32 + order[0] * wc);
        __builtin_prefetch(src + i + 32 + order[1] * wc);
        __builtin_prefetch(src + i + 32 + order[2] * wc);
        __builtin_prefetch(src + i + 32 + order[3] * wc);
        __m256i v0 =
            _mm256_loadu_si256((const __m256i *)(src + i + order[0] * wc));
        __m256i v1 =
            _mm256_loadu_si256((const __m256i *)(src + i + order[1] * wc));
        __m256i v2 =
            _mm256_loadu_si256((const __m256i *)(src + i + order[2] * wc));
        __m256i v3 =
            _mm256_loadu_si256((const __m256i *)(src + i + order[3] * wc));
        __m256i t0 = _mm256_unpacklo_epi8(v0, v1);
        __m256i t1 = _mm256_unpackhi_epi8(v0, v1);
        __m256i t2 = _mm256_unpacklo_epi8(v2, v3);
        __m256i t3 = _mm256_unpackhi_epi8(v2, v3);
        __m256i r0 = _mm256_unpacklo_epi16(t0, t2);
        __m256i r1 = _mm256_unpackhi_epi16(t0, t2);
        __m256i r2 = _mm256_unpacklo_epi16(t1, t3);
        __m256i r3 = _mm256_unpackhi_epi16(t1, t3);
        _mm256_storeu_si256((__m256i *)(dst + 4 * i + 0 * 32), r0);
        _mm256_storeu_si256((__m256i *)(dst + 4 * i + 1 * 32), r1);
        _mm256_storeu_si256((__m256i *)(dst + 4 * i + 2 * 32), r2);
        _mm256_storeu_si256((__m256i *)(dst + 4 * i + 3 * 32), r3);
    }
    for (; i < wc; i++)
    {
        for (int b = 0; b < 4; b++)
            dst[4 * i + b] = src[i + order[b] * wc];
    }
}

static void interleave8_avx2(uint8_t *dst, const uint8_t *src, tmsize_t wc,
                             const int oh[4], const int ol[4])
{
    tmsize_t i = 0;
    for (; i + 31 < wc; i += 32)
    {
        interleave4_avx2(dst + 8 * i, src + 0, wc, oh);
        interleave4_avx2(dst + 8 * i + 4 * 32, src + 0, wc, ol);
    }
    for (; i < wc; i++)
    {
        for (int b = 0; b < 8; b++)
            dst[8 * i + b] = src[i + (b < 4 ? oh[b] : ol[b - 4]) * wc];
    }
}
#endif

#if defined(HAVE_NEON) && defined(__ARM_NEON)
static void interleave4_neon(uint8_t *dst, const uint8_t *src, tmsize_t wc,
                             const int order[4])
{
    tmsize_t i = 0;
    for (; i + 16 <= wc; i += 16)
    {
        __builtin_prefetch(src + i + 32 + order[0] * wc);
        __builtin_prefetch(src + i + 32 + order[1] * wc);
        __builtin_prefetch(src + i + 32 + order[2] * wc);
        __builtin_prefetch(src + i + 32 + order[3] * wc);
        uint8x16_t v0 = vld1q_u8(src + i + order[0] * wc);
        uint8x16_t v1 = vld1q_u8(src + i + order[1] * wc);
        uint8x16_t v2 = vld1q_u8(src + i + order[2] * wc);
        uint8x16_t v3 = vld1q_u8(src + i + order[3] * wc);
        uint8x16x4_t v = {v0, v1, v2, v3};
        vst4q_u8(dst + 4 * i, v);
    }
    for (; i < wc; i++)
    {
        for (int b = 0; b < 4; b++)
            dst[4 * i + b] = src[i + order[b] * wc];
    }
}

static void interleave8_neon(uint8_t *dst, const uint8_t *src, tmsize_t wc,
                             const int oh[4], const int ol[4])
{
    tmsize_t i = 0;
    for (; i + 16 <= wc; i += 16)
    {
        interleave4_neon(dst + 8 * i, src, wc, oh);
        interleave4_neon(dst + 8 * i + 4 * 16, src, wc, ol);
    }
    for (; i < wc; i++)
    {
        for (int b = 0; b < 8; b++)
            dst[8 * i + b] = src[i + (b < 4 ? oh[b] : ol[b - 4]) * wc];
    }
}
#endif
static int PredictorDecodeRow(TIFF *tif, uint8_t *op0, tmsize_t occ0,
                              uint16_t s);
static int PredictorDecodeTile(TIFF *tif, uint8_t *op0, tmsize_t occ0,
                               uint16_t s);
static int PredictorEncodeRow(TIFF *tif, uint8_t *bp, tmsize_t cc, uint16_t s);
static int PredictorEncodeTile(TIFF *tif, uint8_t *bp0, tmsize_t cc0,
                               uint16_t s);

static int PredictorSetup(TIFF *tif)
{
    static const char module[] = "PredictorSetup";

    TIFFPredictorState *sp = PredictorState(tif);
    TIFFDirectory *td = &tif->tif_dir;

    switch (sp->predictor) /* no differencing */
    {
        case PREDICTOR_NONE:
            return 1;
        case PREDICTOR_HORIZONTAL:
            if (td->td_bitspersample != 8 && td->td_bitspersample != 16 &&
                td->td_bitspersample != 32 && td->td_bitspersample != 64)
            {
                TIFFErrorExtR(tif, module,
                              "Horizontal differencing \"Predictor\" not "
                              "supported with %" PRIu16 "-bit samples",
                              td->td_bitspersample);
                return 0;
            }
            break;
        case PREDICTOR_FLOATINGPOINT:
            if (td->td_sampleformat != SAMPLEFORMAT_IEEEFP)
            {
                TIFFErrorExtR(
                    tif, module,
                    "Floating point \"Predictor\" not supported with %" PRIu16
                    " data format",
                    td->td_sampleformat);
                return 0;
            }
            if (td->td_bitspersample != 16 && td->td_bitspersample != 24 &&
                td->td_bitspersample != 32 && td->td_bitspersample != 64)
            { /* Should 64 be allowed? */
                TIFFErrorExtR(
                    tif, module,
                    "Floating point \"Predictor\" not supported with %" PRIu16
                    "-bit samples",
                    td->td_bitspersample);
                return 0;
            }
            break;
        default:
            TIFFErrorExtR(tif, module, "\"Predictor\" value %d not supported",
                          sp->predictor);
            return 0;
    }
    sp->stride =
        (td->td_planarconfig == PLANARCONFIG_CONTIG ? td->td_samplesperpixel
                                                    : 1);
    /*
     * Calculate the scanline/tile-width size in bytes.
     */
    if (isTiled(tif))
        sp->rowsize = TIFFTileRowSize(tif);
    else
        sp->rowsize = TIFFScanlineSize(tif);
    if (sp->rowsize == 0)
        return 0;

    return 1;
}

static int PredictorSetupDecode(TIFF *tif)
{
    TIFFPredictorState *sp = PredictorState(tif);
    TIFFDirectory *td = &tif->tif_dir;

    /* Note: when PredictorSetup() fails, the effets of setupdecode() */
    /* will not be "canceled" so setupdecode() might be robust to */
    /* be called several times. */
    if (!(*sp->setupdecode)(tif) || !PredictorSetup(tif))
        return 0;

    if (sp->predictor == 2)
    {
        switch (td->td_bitspersample)
        {
            case 8:
                sp->decodepfunc = horAcc8;
                break;
            case 16:
                sp->decodepfunc = horAcc16;
                break;
            case 32:
                sp->decodepfunc = horAcc32;
                break;
            case 64:
                sp->decodepfunc = horAcc64;
                break;
        }
        /*
         * Override default decoding method with one that does the
         * predictor stuff.
         */
        if (tif->tif_decoderow != PredictorDecodeRow)
        {
            sp->decoderow = tif->tif_decoderow;
            tif->tif_decoderow = PredictorDecodeRow;
            sp->decodestrip = tif->tif_decodestrip;
            tif->tif_decodestrip = PredictorDecodeTile;
            sp->decodetile = tif->tif_decodetile;
            tif->tif_decodetile = PredictorDecodeTile;
        }

        /*
         * If the data is horizontally differenced 16-bit data that
         * requires byte-swapping, then it must be byte swapped before
         * the accumulation step.  We do this with a special-purpose
         * routine and override the normal post decoding logic that
         * the library setup when the directory was read.
         */
        if (tif->tif_flags & TIFF_SWAB)
        {
            if (sp->decodepfunc == horAcc16)
            {
                sp->decodepfunc = swabHorAcc16;
                tif->tif_postdecode = _TIFFNoPostDecode;
            }
            else if (sp->decodepfunc == horAcc32)
            {
                sp->decodepfunc = swabHorAcc32;
                tif->tif_postdecode = _TIFFNoPostDecode;
            }
            else if (sp->decodepfunc == horAcc64)
            {
                sp->decodepfunc = swabHorAcc64;
                tif->tif_postdecode = _TIFFNoPostDecode;
            }
        }
    }

    else if (sp->predictor == 3)
    {
        sp->decodepfunc = fpAcc;
        /*
         * Override default decoding method with one that does the
         * predictor stuff.
         */
        if (tif->tif_decoderow != PredictorDecodeRow)
        {
            sp->decoderow = tif->tif_decoderow;
            tif->tif_decoderow = PredictorDecodeRow;
            sp->decodestrip = tif->tif_decodestrip;
            tif->tif_decodestrip = PredictorDecodeTile;
            sp->decodetile = tif->tif_decodetile;
            tif->tif_decodetile = PredictorDecodeTile;
        }
        /*
         * The data should not be swapped outside of the floating
         * point predictor, the accumulation routine should return
         * bytes in the native order.
         */
        if (tif->tif_flags & TIFF_SWAB)
        {
            tif->tif_postdecode = _TIFFNoPostDecode;
        }
    }

    return 1;
}

static int PredictorSetupEncode(TIFF *tif)
{
    TIFFPredictorState *sp = PredictorState(tif);
    TIFFDirectory *td = &tif->tif_dir;

    if (!(*sp->setupencode)(tif) || !PredictorSetup(tif))
        return 0;

    if (sp->predictor == 2)
    {
        switch (td->td_bitspersample)
        {
            case 8:
                sp->encodepfunc = horDiff8;
                break;
            case 16:
                sp->encodepfunc = horDiff16;
                break;
            case 32:
                sp->encodepfunc = horDiff32;
                break;
            case 64:
                sp->encodepfunc = horDiff64;
                break;
        }
        /*
         * Override default encoding method with one that does the
         * predictor stuff.
         */
        if (tif->tif_encoderow != PredictorEncodeRow)
        {
            sp->encoderow = tif->tif_encoderow;
            tif->tif_encoderow = PredictorEncodeRow;
            sp->encodestrip = tif->tif_encodestrip;
            tif->tif_encodestrip = PredictorEncodeTile;
            sp->encodetile = tif->tif_encodetile;
            tif->tif_encodetile = PredictorEncodeTile;
        }

        /*
         * If the data is horizontally differenced 16-bit data that
         * requires byte-swapping, then it must be byte swapped after
         * the differentiation step.  We do this with a special-purpose
         * routine and override the normal post decoding logic that
         * the library setup when the directory was read.
         */
        if (tif->tif_flags & TIFF_SWAB)
        {
            if (sp->encodepfunc == horDiff16)
            {
                sp->encodepfunc = swabHorDiff16;
                tif->tif_postdecode = _TIFFNoPostDecode;
            }
            else if (sp->encodepfunc == horDiff32)
            {
                sp->encodepfunc = swabHorDiff32;
                tif->tif_postdecode = _TIFFNoPostDecode;
            }
            else if (sp->encodepfunc == horDiff64)
            {
                sp->encodepfunc = swabHorDiff64;
                tif->tif_postdecode = _TIFFNoPostDecode;
            }
        }
    }

    else if (sp->predictor == 3)
    {
        sp->encodepfunc = fpDiff;
        /*
         * Override default encoding method with one that does the
         * predictor stuff.
         */
        if (tif->tif_encoderow != PredictorEncodeRow)
        {
            sp->encoderow = tif->tif_encoderow;
            tif->tif_encoderow = PredictorEncodeRow;
            sp->encodestrip = tif->tif_encodestrip;
            tif->tif_encodestrip = PredictorEncodeTile;
            sp->encodetile = tif->tif_encodetile;
            tif->tif_encodetile = PredictorEncodeTile;
        }
        /*
         * The data should not be swapped outside of the floating
         * point predictor, the differentiation routine should return
         * bytes in the native order.
         */
        if (tif->tif_flags & TIFF_SWAB)
        {
            tif->tif_postdecode = _TIFFNoPostDecode;
        }
    }

    return 1;
}

#define REPEAT4(n, op)                                                         \
    switch (n)                                                                 \
    {                                                                          \
        default:                                                               \
        {                                                                      \
            tmsize_t i;                                                        \
            for (i = n - 4; i > 0; i--)                                        \
            {                                                                  \
                op;                                                            \
            }                                                                  \
        } /*-fallthrough*/                                                     \
        case 4:                                                                \
            op; /*-fallthrough*/                                               \
        case 3:                                                                \
            op; /*-fallthrough*/                                               \
        case 2:                                                                \
            op; /*-fallthrough*/                                               \
        case 1:                                                                \
            op; /*-fallthrough*/                                               \
        case 0:;                                                               \
    }

/* Remarks related to C standard compliance in all below functions : */
/* - to avoid any undefined behavior, we only operate on unsigned types */
/*   since the behavior of "overflows" is defined (wrap over) */
/* - when storing into the byte stream, we explicitly mask with 0xff so */
/*   as to make icc -check=conversions happy (not necessary by the standard) */

TIFF_NOSANITIZE_UNSIGNED_INT_OVERFLOW
static int horAcc8(TIFF *tif, uint8_t *cp0, tmsize_t cc)
{
    tmsize_t stride = PredictorState(tif)->stride;

    uint8_t *cp = cp0;
    if ((cc % stride) != 0)
    {
        TIFFErrorExtR(tif, "horAcc8", "%s", "(cc%stride)!=0");
        return 0;
    }

    if (cc > stride)
    {
        /*
         * Pipeline the most common cases.
         */
        if (stride == 1)
        {
            uint32_t acc = cp[0];
            tmsize_t i = stride;
#if defined(HAVE_NEON) && defined(__ARM_NEON)
            if (cc - i >= 16)
            {
                uint8_t *p = cp + i;
                tmsize_t remaining = cc - i;
                uint8_t acc8 = (uint8_t)acc;
                while (remaining >= 16)
                {
                    __builtin_prefetch(p + 16);
                    uint8x16_t v = vld1q_u8(p);
                    __builtin_prefetch(p + 64);
                    v = vaddq_u8(v, vextq_u8(vdupq_n_u8(0), v, 15));
                    v = vaddq_u8(v, vextq_u8(vdupq_n_u8(0), v, 14));
                    v = vaddq_u8(v, vextq_u8(vdupq_n_u8(0), v, 12));
                    v = vaddq_u8(v, vextq_u8(vdupq_n_u8(0), v, 8));
                    v = vaddq_u8(v, vdupq_n_u8(acc8));
                    vst1q_u8(p, v);
                    acc8 = vgetq_lane_u8(v, 15);
                    p += 16;
                    remaining -= 16;
                }
                acc = acc8;
                i = cc - remaining;
            }
#endif
#if defined(HAVE_SSE41)
            if (cc - i >= 16)
            {
                uint8_t *p = cp + i;
                tmsize_t remaining = cc - i;
                uint8_t acc8 = (uint8_t)acc;
                while (remaining >= 16)
                {
                    __builtin_prefetch(p + 16);
                    __m128i v = _mm_loadu_si128((const __m128i *)p);
                    _mm_prefetch((const char *)(p + 64), _MM_HINT_T0);
                    v = _mm_add_epi8(v, _mm_slli_si128(v, 1));
                    v = _mm_add_epi8(v, _mm_slli_si128(v, 2));
                    v = _mm_add_epi8(v, _mm_slli_si128(v, 4));
                    v = _mm_add_epi8(v, _mm_slli_si128(v, 8));
                    v = _mm_add_epi8(v, _mm_set1_epi8((char)acc8));
                    _mm_storeu_si128((__m128i *)p, v);
                    acc8 = (uint8_t)_mm_extract_epi8(v, 15);
                    p += 16;
                    remaining -= 16;
                }
                acc = acc8;
                i = cc - remaining;
            }
#endif
#if defined(HAVE_SSE2) && !defined(HAVE_SSE41)
            if (cc - i >= 16)
            {
                uint8_t *p = cp + i;
                tmsize_t remaining = cc - i;
                uint8_t acc8 = (uint8_t)acc;
                while (remaining >= 16)
                {
                    __builtin_prefetch(p + 16);
                    __m128i v = _mm_loadu_si128((const __m128i *)p);
                    _mm_prefetch((const char *)(p + 64), _MM_HINT_T0);
                    v = _mm_add_epi8(v, _mm_slli_si128(v, 1));
                    v = _mm_add_epi8(v, _mm_slli_si128(v, 2));
                    v = _mm_add_epi8(v, _mm_slli_si128(v, 4));
                    v = _mm_add_epi8(v, _mm_slli_si128(v, 8));
                    v = _mm_add_epi8(v, _mm_set1_epi8((char)acc8));
                    _mm_storeu_si128((__m128i *)p, v);
                    acc8 = (uint8_t)_mm_cvtsi128_si32(_mm_srli_si128(v, 15));
                    p += 16;
                    remaining -= 16;
                }
                acc = acc8;
                i = cc - remaining;
            }
#endif
            for (; i < cc; i++)
            {
                cp[i] = (uint8_t)((acc += cp[i]) & 0xff);
            }
        }
        else if (stride == 3)
        {
            uint32_t cr = cp[0];
            uint32_t cg = cp[1];
            uint32_t cb = cp[2];
            tmsize_t i = stride;
            for (; i < cc; i += stride)
            {
                cp[i + 0] = (uint8_t)((cr += cp[i + 0]) & 0xff);
                cp[i + 1] = (uint8_t)((cg += cp[i + 1]) & 0xff);
                cp[i + 2] = (uint8_t)((cb += cp[i + 2]) & 0xff);
            }
        }
        else if (stride == 4)
        {
            uint32_t cr = cp[0];
            uint32_t cg = cp[1];
            uint32_t cb = cp[2];
            uint32_t ca = cp[3];
            tmsize_t i = stride;
            for (; i < cc; i += stride)
            {
                cp[i + 0] = (uint8_t)((cr += cp[i + 0]) & 0xff);
                cp[i + 1] = (uint8_t)((cg += cp[i + 1]) & 0xff);
                cp[i + 2] = (uint8_t)((cb += cp[i + 2]) & 0xff);
                cp[i + 3] = (uint8_t)((ca += cp[i + 3]) & 0xff);
            }
        }
        else
        {
            cc -= stride;
            do
            {
                REPEAT4(stride,
                        cp[stride] = (uint8_t)((cp[stride] + *cp) & 0xff);
                        cp++)
                cc -= stride;
            } while (cc > 0);
        }
    }
    return 1;
}

static int swabHorAcc16(TIFF *tif, uint8_t *cp0, tmsize_t cc)
{
    uint16_t *wp = (uint16_t *)cp0;
    tmsize_t wc = cc / 2;

    TIFFSwabArrayOfShort(wp, wc);
    return horAcc16(tif, cp0, cc);
}

TIFF_NOSANITIZE_UNSIGNED_INT_OVERFLOW
static int horAcc16(TIFF *tif, uint8_t *cp0, tmsize_t cc)
{
    tmsize_t stride = PredictorState(tif)->stride;
    uint16_t *wp = (uint16_t *)cp0;
    tmsize_t wc = cc / 2;

    if ((cc % (2 * stride)) != 0)
    {
        TIFFErrorExtR(tif, "horAcc16", "%s", "cc%(2*stride))!=0");
        return 0;
    }

    if (wc > stride)
    {
        if (stride == 1)
        {
            uint32_t acc = wp[0];
            tmsize_t i = stride;
#if defined(HAVE_NEON) && defined(__ARM_NEON)
            if (wc - i >= 8)
            {
                uint16_t *p = wp + i;
                tmsize_t remaining = wc - i;
                uint16_t acc16 = (uint16_t)acc;
                while (remaining >= 8)
                {
                    __builtin_prefetch(p + 16);
                    uint16x8_t v = vld1q_u16(p);
                    v = vqaddq_u16(v, vextq_u16(vdupq_n_u16(0), v, 7));
                    v = vqaddq_u16(v, vextq_u16(vdupq_n_u16(0), v, 6));
                    v = vqaddq_u16(v, vextq_u16(vdupq_n_u16(0), v, 4));
                    v = vqaddq_u16(v, vdupq_n_u16(acc16));
                    vst1q_u16(p, v);
                    acc16 = vgetq_lane_u16(v, 7);
                    p += 8;
                    remaining -= 8;
                }
                acc = acc16;
                i = wc - remaining;
            }
#endif
#if defined(HAVE_SSE41)
            if (wc - i >= 8)
            {
                uint16_t *p = wp + i;
                tmsize_t remaining = wc - i;
                uint16_t acc16 = (uint16_t)acc;
                while (remaining >= 8)
                {
                    __builtin_prefetch(p + 16);
                    __m128i v = _mm_loadu_si128((const __m128i *)p);
                    _mm_prefetch((const char *)(p + 32), _MM_HINT_T0);
                    v = _mm_adds_epu16(v, _mm_slli_si128(v, 2));
                    v = _mm_adds_epu16(v, _mm_slli_si128(v, 4));
                    v = _mm_adds_epu16(v, _mm_slli_si128(v, 8));
                    v = _mm_adds_epu16(v, _mm_set1_epi16(acc16));
                    _mm_storeu_si128((__m128i *)p, v);
                    acc16 = (uint16_t)_mm_extract_epi16(v, 7);
                    p += 8;
                    remaining -= 8;
                }
                acc = acc16;
                i = wc - remaining;
            }
#endif
#if defined(HAVE_SSE2) && !defined(HAVE_SSE41)
            if (wc - i >= 8)
            {
                uint16_t *p = wp + i;
                tmsize_t remaining = wc - i;
                uint16_t acc16 = (uint16_t)acc;
                while (remaining >= 8)
                {
                    __builtin_prefetch(p + 16);
                    __m128i v = _mm_loadu_si128((const __m128i *)p);
                    _mm_prefetch((const char *)(p + 32), _MM_HINT_T0);
                    v = _mm_adds_epu16(v, _mm_slli_si128(v, 2));
                    v = _mm_adds_epu16(v, _mm_slli_si128(v, 4));
                    v = _mm_adds_epu16(v, _mm_slli_si128(v, 8));
                    v = _mm_adds_epu16(v, _mm_set1_epi16(acc16));
                    _mm_storeu_si128((__m128i *)p, v);
                    acc16 = (uint16_t)_mm_cvtsi128_si32(_mm_srli_si128(v, 14));
                    p += 8;
                    remaining -= 8;
                }
                acc = acc16;
                i = wc - remaining;
            }
#endif
            for (; i < wc; i++)
            {
                acc = add_sat_u16((uint16_t)acc, wp[i]);
                wp[i] = (uint16_t)acc;
            }
        }
        else
        {
            wc -= stride;
            do
            {
                REPEAT4(stride, wp[stride] = add_sat_u16(wp[stride], wp[0]);
                        wp++)
                wc -= stride;
            } while (wc > 0);
        }
    }
    return 1;
}

static int swabHorAcc32(TIFF *tif, uint8_t *cp0, tmsize_t cc)
{
    uint32_t *wp = (uint32_t *)cp0;
    tmsize_t wc = cc / 4;

    TIFFSwabArrayOfLong(wp, wc);
    return horAcc32(tif, cp0, cc);
}

TIFF_NOSANITIZE_UNSIGNED_INT_OVERFLOW
static int horAcc32(TIFF *tif, uint8_t *cp0, tmsize_t cc)
{
    tmsize_t stride = PredictorState(tif)->stride;
    uint32_t *wp = (uint32_t *)cp0;
    tmsize_t wc = cc / 4;

    if ((cc % (4 * stride)) != 0)
    {
        TIFFErrorExtR(tif, "horAcc32", "%s", "cc%(4*stride))!=0");
        return 0;
    }

    if (wc > stride)
    {
        wc -= stride;
        do
        {
            REPEAT4(stride, wp[stride] += wp[0]; wp++)
            wc -= stride;
        } while (wc > 0);
    }
    return 1;
}

static int swabHorAcc64(TIFF *tif, uint8_t *cp0, tmsize_t cc)
{
    uint64_t *wp = (uint64_t *)cp0;
    tmsize_t wc = cc / 8;

    TIFFSwabArrayOfLong8(wp, wc);
    return horAcc64(tif, cp0, cc);
}

TIFF_NOSANITIZE_UNSIGNED_INT_OVERFLOW
static int horAcc64(TIFF *tif, uint8_t *cp0, tmsize_t cc)
{
    tmsize_t stride = PredictorState(tif)->stride;
    uint64_t *wp = (uint64_t *)cp0;
    tmsize_t wc = cc / 8;

    if ((cc % (8 * stride)) != 0)
    {
        TIFFErrorExtR(tif, "horAcc64", "%s", "cc%(8*stride))!=0");
        return 0;
    }

    if (wc > stride)
    {
        wc -= stride;
        do
        {
            REPEAT4(stride, wp[stride] += wp[0]; wp++)
            wc -= stride;
        } while (wc > 0);
    }
    return 1;
}

/*
 * Floating point predictor accumulation routine.
 */
static int fpAcc(TIFF *tif, uint8_t *cp0, tmsize_t cc)
{
    tmsize_t stride = PredictorState(tif)->stride;
    uint32_t bps = tif->tif_dir.td_bitspersample / 8;
    tmsize_t wc = cc / bps;
    tmsize_t count = cc;
    uint8_t *cp = cp0;
    uint8_t *tmp;

    if (cc % (bps * stride) != 0)
    {
        TIFFErrorExtR(tif, "fpAcc", "%s", "cc%(bps*stride))!=0");
        return 0;
    }

    tmp = (uint8_t *)_TIFFmallocExt(tif, cc);
    if (!tmp)
        return 0;

    if (stride == 1)
    {
        /* Optimization of general case */
#define OP                                                                     \
    do                                                                         \
    {                                                                          \
        cp[1] = (uint8_t)((cp[1] + cp[0]) & 0xff);                             \
        ++cp;                                                                  \
    } while (0)
        for (; count > 8; count -= 8)
        {
            OP;
            OP;
            OP;
            OP;
            OP;
            OP;
            OP;
            OP;
        }
        for (; count > 1; count -= 1)
        {
            OP;
        }
#undef OP
    }
    else
    {
        while (count > stride)
        {
            REPEAT4(stride, cp[stride] = (uint8_t)((cp[stride] + cp[0]) & 0xff);
                    cp++)
            count -= stride;
        }
    }

    _TIFFmemcpy(tmp, cp0, cc);
    cp = (uint8_t *)cp0;
    count = 0;

#if defined(__AVX2__)
    if (bps == 4 && stride == 1)
    {
        const int order[4] =
#if WORDS_BIGENDIAN
            {0, 1, 2, 3};
#else
            {3, 2, 1, 0};
#endif
        interleave4_avx2(cp, tmp, wc, order);
        count = wc;
    }
    else if (bps == 8 && stride == 1)
    {
        const int oh[4] =
#if WORDS_BIGENDIAN
            {0, 1, 2, 3};
#else
            {7, 6, 5, 4};
#endif
        const int ol[4] =
#if WORDS_BIGENDIAN
            {4, 5, 6, 7};
#else
            {3, 2, 1, 0};
#endif
        interleave8_avx2(cp, tmp, wc, oh, ol);
        count = wc;
    }
#endif
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    if (count == 0 && bps == 4 && stride == 1)
    {
        const int order[4] =
#if WORDS_BIGENDIAN
            {0, 1, 2, 3};
#else
            {3, 2, 1, 0};
#endif
        interleave4_neon(cp, tmp, wc, order);
        count = wc;
    }
    else if (count == 0 && bps == 8 && stride == 1)
    {
        const int oh[4] =
#if WORDS_BIGENDIAN
            {0, 1, 2, 3};
#else
            {7, 6, 5, 4};
#endif
        const int ol[4] =
#if WORDS_BIGENDIAN
            {4, 5, 6, 7};
#else
            {3, 2, 1, 0};
#endif
        interleave8_neon(cp, tmp, wc, oh, ol);
        count = wc;
    }
#endif
#if defined(__x86_64__) || defined(_M_X64)
    if (count == 0 && bps == 4)
    {
        /* Optimization of general case */
        for (; count + 15 < wc; count += 16)
        {
            __m128i xmm0 = _mm_loadu_si128((const __m128i *)(tmp + count +
#if WORDS_BIGENDIAN
                                                             0
#else
                                                             3
#endif
                                                                 * wc));
            __m128i xmm1 = _mm_loadu_si128((const __m128i *)(tmp + count +
#if WORDS_BIGENDIAN
                                                             1
#else
                                                             2
#endif
                                                                 * wc));
            __m128i xmm2 = _mm_loadu_si128((const __m128i *)(tmp + count +
#if WORDS_BIGENDIAN
                                                             2
#else
                                                             1
#endif
                                                                 * wc));
            __m128i xmm3 = _mm_loadu_si128((const __m128i *)(tmp + count +
#if WORDS_BIGENDIAN
                                                             3
#else
                                                             0
#endif
                                                                 * wc));
            __m128i tmp0 = _mm_unpacklo_epi8(xmm0, xmm1);
            __m128i tmp1 = _mm_unpackhi_epi8(xmm0, xmm1);
            __m128i tmp2 = _mm_unpacklo_epi8(xmm2, xmm3);
            __m128i tmp3 = _mm_unpackhi_epi8(xmm2, xmm3);
            __m128i tmp2_0 = _mm_unpacklo_epi16(tmp0, tmp2);
            __m128i tmp2_1 = _mm_unpackhi_epi16(tmp0, tmp2);
            __m128i tmp2_2 = _mm_unpacklo_epi16(tmp1, tmp3);
            __m128i tmp2_3 = _mm_unpackhi_epi16(tmp1, tmp3);
            _mm_storeu_si128((__m128i *)(cp + 4 * count + 0 * 16), tmp2_0);
            _mm_storeu_si128((__m128i *)(cp + 4 * count + 1 * 16), tmp2_1);
            _mm_storeu_si128((__m128i *)(cp + 4 * count + 2 * 16), tmp2_2);
            _mm_storeu_si128((__m128i *)(cp + 4 * count + 3 * 16), tmp2_3);
        }
    }
#endif

    for (; count < wc; count++)
    {
        uint32_t byte;
        for (byte = 0; byte < bps; byte++)
        {
#if WORDS_BIGENDIAN
            cp[bps * count + byte] = tmp[byte * wc + count];
#else
            cp[bps * count + byte] = tmp[(bps - byte - 1) * wc + count];
#endif
        }
    }
    _TIFFfreeExt(tif, tmp);
    return 1;
}

/*
 * Decode a scanline and apply the predictor routine.
 */
static int PredictorDecodeRow(TIFF *tif, uint8_t *op0, tmsize_t occ0,
                              uint16_t s)
{
    TIFFPredictorState *sp = PredictorState(tif);

    assert(sp != NULL);
    assert(sp->decoderow != NULL);
    assert(sp->decodepfunc != NULL);

    if ((*sp->decoderow)(tif, op0, occ0, s))
    {
        return (*sp->decodepfunc)(tif, op0, occ0);
    }
    else
        return 0;
}

/*
 * Decode a tile/strip and apply the predictor routine.
 * Note that horizontal differencing must be done on a
 * row-by-row basis.  The width of a "row" has already
 * been calculated at pre-decode time according to the
 * strip/tile dimensions.
 */
static int PredictorDecodeTile(TIFF *tif, uint8_t *op0, tmsize_t occ0,
                               uint16_t s)
{
    TIFFPredictorState *sp = PredictorState(tif);

    assert(sp != NULL);
    assert(sp->decodetile != NULL);

    if ((*sp->decodetile)(tif, op0, occ0, s))
    {
        tmsize_t rowsize = sp->rowsize;
        assert(rowsize > 0);
        if ((occ0 % rowsize) != 0)
        {
            TIFFErrorExtR(tif, "PredictorDecodeTile", "%s",
                          "occ0%rowsize != 0");
            return 0;
        }
        assert(sp->decodepfunc != NULL);
        while (occ0 > 0)
        {
            if (!(*sp->decodepfunc)(tif, op0, rowsize))
                return 0;
            occ0 -= rowsize;
            op0 += rowsize;
        }
        return 1;
    }
    else
        return 0;
}

TIFF_NOSANITIZE_UNSIGNED_INT_OVERFLOW
static int horDiff8(TIFF *tif, uint8_t *cp0, tmsize_t cc)
{
    TIFFPredictorState *sp = PredictorState(tif);
    tmsize_t stride = sp->stride;
    unsigned char *cp = (unsigned char *)cp0;

    if ((cc % stride) != 0)
    {
        TIFFErrorExtR(tif, "horDiff8", "%s", "(cc%stride)!=0");
        return 0;
    }

    if (cc > stride)
    {
        cc -= stride;
        /*
         * Pipeline the most common cases.
         */
        if (stride == 1)
        {
#if defined(HAVE_NEON) && defined(__ARM_NEON)
            if (cc >= 16)
            {
                uint8_t *p = cp + 1;
                tmsize_t remaining = cc - 1;
                while (remaining >= 32)
                {
                    __builtin_prefetch(p + 16);
                    uint8x16_t cur1 = vld1q_u8(p);
                    uint8x16_t cur2 = vld1q_u8(p + 16);
                    __builtin_prefetch(p + 128);
                    uint8x16_t prev1 = vld1q_u8(p - 1);
                    uint8x16_t prev2 = vld1q_u8(p + 15);
                    vst1q_u8(p, vsubq_u8(cur1, prev1));
                    vst1q_u8(p + 16, vsubq_u8(cur2, prev2));
                    p += 32;
                    remaining -= 32;
                }
                while (remaining >= 16)
                {
                    __builtin_prefetch(p + 16);
                    uint8x16_t cur = vld1q_u8(p);
                    __builtin_prefetch(p + 64);
                    uint8x16_t prev = vld1q_u8(p - 1);
                    vst1q_u8(p, vsubq_u8(cur, prev));
                    p += 16;
                    remaining -= 16;
                }
                while (remaining--)
                {
                    *p = (uint8_t)(*p - *(p - 1));
                    ++p;
                }
                return 1;
            }
#endif
        }
        if (stride == 3)
        {
            unsigned int r1, g1, b1;
            unsigned int r2 = cp[0];
            unsigned int g2 = cp[1];
            unsigned int b2 = cp[2];
            do
            {
                r1 = cp[3];
                cp[3] = (unsigned char)((r1 - r2) & 0xff);
                r2 = r1;
                g1 = cp[4];
                cp[4] = (unsigned char)((g1 - g2) & 0xff);
                g2 = g1;
                b1 = cp[5];
                cp[5] = (unsigned char)((b1 - b2) & 0xff);
                b2 = b1;
                cp += 3;
            } while ((cc -= 3) > 0);
        }
        else if (stride == 4)
        {
            unsigned int r1, g1, b1, a1;
            unsigned int r2 = cp[0];
            unsigned int g2 = cp[1];
            unsigned int b2 = cp[2];
            unsigned int a2 = cp[3];
            do
            {
                r1 = cp[4];
                cp[4] = (unsigned char)((r1 - r2) & 0xff);
                r2 = r1;
                g1 = cp[5];
                cp[5] = (unsigned char)((g1 - g2) & 0xff);
                g2 = g1;
                b1 = cp[6];
                cp[6] = (unsigned char)((b1 - b2) & 0xff);
                b2 = b1;
                a1 = cp[7];
                cp[7] = (unsigned char)((a1 - a2) & 0xff);
                a2 = a1;
                cp += 4;
            } while ((cc -= 4) > 0);
        }
        else
        {
            cp += cc - 1;
            do
            {
                REPEAT4(stride,
                        cp[stride] =
                            (unsigned char)((cp[stride] - cp[0]) & 0xff);
                        cp--)
            } while ((cc -= stride) > 0);
        }
    }
    return 1;
}

TIFF_NOSANITIZE_UNSIGNED_INT_OVERFLOW
static int horDiff16(TIFF *tif, uint8_t *cp0, tmsize_t cc)
{
    TIFFPredictorState *sp = PredictorState(tif);
    tmsize_t stride = sp->stride;
    uint16_t *wp = (uint16_t *)cp0;
    tmsize_t wc = cc / 2;

    if ((cc % (2 * stride)) != 0)
    {
        TIFFErrorExtR(tif, "horDiff8", "%s", "(cc%(2*stride))!=0");
        return 0;
    }

    if (wc > stride)
    {
        wc -= stride;
        wp += wc - 1;
#if defined(HAVE_NEON) && defined(__ARM_NEON)
        if (stride == 1 && wc >= 8)
        {
            uint16_t *p = wp + 1;
            tmsize_t remaining = wc - 1;
            while (remaining >= 16)
            {
                __builtin_prefetch(p + 16);
                uint16x8_t cur0 = vld1q_u16(p);
                uint16x8_t cur1 = vld1q_u16(p + 8);
                __builtin_prefetch(p + 32);
                uint16x8_t prev0 = vld1q_u16(p - 1);
                uint16x8_t prev1 = vld1q_u16(p + 7);
                vst1q_u16(p, vsubq_u16(cur0, prev0));
                vst1q_u16(p + 8, vsubq_u16(cur1, prev1));
                p += 16;
                remaining -= 16;
            }
            if (remaining >= 8)
            {
                __builtin_prefetch(p + 16);
                uint16x8_t cur = vld1q_u16(p);
                uint16x8_t prev = vld1q_u16(p - 1);
                vst1q_u16(p, vsubq_u16(cur, prev));
                p += 8;
                remaining -= 8;
            }
            while (remaining--)
            {
                *p = (uint16_t)(*p - *(p - 1));
                ++p;
            }
            return 1;
        }
#endif
        do
        {
            REPEAT4(stride, wp[stride] = (uint16_t)(((unsigned int)wp[stride] -
                                                     (unsigned int)wp[0]) &
                                                    0xffff);
                    wp--)
            wc -= stride;
        } while (wc > 0);
    }
    return 1;
}

static int swabHorDiff16(TIFF *tif, uint8_t *cp0, tmsize_t cc)
{
    uint16_t *wp = (uint16_t *)cp0;
    tmsize_t wc = cc / 2;

    if (!horDiff16(tif, cp0, cc))
        return 0;

    TIFFSwabArrayOfShort(wp, wc);
    return 1;
}

TIFF_NOSANITIZE_UNSIGNED_INT_OVERFLOW
static int horDiff32(TIFF *tif, uint8_t *cp0, tmsize_t cc)
{
    TIFFPredictorState *sp = PredictorState(tif);
    tmsize_t stride = sp->stride;
    uint32_t *wp = (uint32_t *)cp0;
    tmsize_t wc = cc / 4;

    if ((cc % (4 * stride)) != 0)
    {
        TIFFErrorExtR(tif, "horDiff32", "%s", "(cc%(4*stride))!=0");
        return 0;
    }

    if (wc > stride)
    {
        wc -= stride;
        wp += wc - 1;
#if defined(HAVE_NEON) && defined(__ARM_NEON)
        if (stride == 1 && wc >= 4)
        {
            uint32_t *p = wp + 1;
            tmsize_t remaining = wc - 1;
            while (remaining >= 4)
            {
                uint32x4_t cur = vld1q_u32(p);
                uint32x4_t prev = vld1q_u32(p - 1);
                uint32x4_t diff = vsubq_u32(cur, prev);
                vst1q_u32(p, diff);
                p += 4;
                remaining -= 4;
            }
            while (remaining--)
            {
                *p = *p - *(p - 1);
                ++p;
            }
            return 1;
        }
#endif
        do
        {
            REPEAT4(stride, wp[stride] -= wp[0]; wp--)
            wc -= stride;
        } while (wc > 0);
    }
    return 1;
}

static int swabHorDiff32(TIFF *tif, uint8_t *cp0, tmsize_t cc)
{
    uint32_t *wp = (uint32_t *)cp0;
    tmsize_t wc = cc / 4;

    if (!horDiff32(tif, cp0, cc))
        return 0;

    TIFFSwabArrayOfLong(wp, wc);
    return 1;
}

TIFF_NOSANITIZE_UNSIGNED_INT_OVERFLOW
static int horDiff64(TIFF *tif, uint8_t *cp0, tmsize_t cc)
{
    TIFFPredictorState *sp = PredictorState(tif);
    tmsize_t stride = sp->stride;
    uint64_t *wp = (uint64_t *)cp0;
    tmsize_t wc = cc / 8;

    if ((cc % (8 * stride)) != 0)
    {
        TIFFErrorExtR(tif, "horDiff64", "%s", "(cc%(8*stride))!=0");
        return 0;
    }

    if (wc > stride)
    {
        wc -= stride;
        wp += wc - 1;
#if defined(HAVE_NEON) && defined(__ARM_NEON)
        if (stride == 1 && wc >= 2)
        {
            uint64_t *p = wp + 1;
            tmsize_t remaining = wc - 1;
            while (remaining >= 2)
            {
                uint64x2_t cur = vld1q_u64(p);
                uint64x2_t prev = vld1q_u64(p - 1);
                uint64x2_t diff = vsubq_u64(cur, prev);
                vst1q_u64(p, diff);
                p += 2;
                remaining -= 2;
            }
            while (remaining--)
            {
                *p = *p - *(p - 1);
                ++p;
            }
            return 1;
        }
#endif
        do
        {
            REPEAT4(stride, wp[stride] -= wp[0]; wp--)
            wc -= stride;
        } while (wc > 0);
    }
    return 1;
}

static int swabHorDiff64(TIFF *tif, uint8_t *cp0, tmsize_t cc)
{
    uint64_t *wp = (uint64_t *)cp0;
    tmsize_t wc = cc / 8;

    if (!horDiff64(tif, cp0, cc))
        return 0;

    TIFFSwabArrayOfLong8(wp, wc);
    return 1;
}

/*
 * Floating point predictor differencing routine.
 */
TIFF_NOSANITIZE_UNSIGNED_INT_OVERFLOW
static int fpDiff(TIFF *tif, uint8_t *cp0, tmsize_t cc)
{
    tmsize_t stride = PredictorState(tif)->stride;
    uint32_t bps = tif->tif_dir.td_bitspersample / 8;
    tmsize_t wc = cc / bps;
    tmsize_t count;
    uint8_t *cp = (uint8_t *)cp0;
    uint8_t *tmp;

    if ((cc % (bps * stride)) != 0)
    {
        TIFFErrorExtR(tif, "fpDiff", "%s", "(cc%(bps*stride))!=0");
        return 0;
    }

    tmp = (uint8_t *)_TIFFmallocExt(tif, cc);
    if (!tmp)
        return 0;

    _TIFFmemcpy(tmp, cp0, cc);
    count = 0;
#if defined(__AVX2__)
    if (bps == 4 && stride == 1)
    {
        const int order[4] =
#if WORDS_BIGENDIAN
            {0, 1, 2, 3};
#else
            {3, 2, 1, 0};
#endif
        interleave4_avx2(cp, tmp, wc, order);
        count = wc;
    }
    else if (bps == 8 && stride == 1)
    {
        const int oh[4] =
#if WORDS_BIGENDIAN
            {0, 1, 2, 3};
#else
            {7, 6, 5, 4};
#endif
        const int ol[4] =
#if WORDS_BIGENDIAN
            {4, 5, 6, 7};
#else
            {3, 2, 1, 0};
#endif
        interleave8_avx2(cp, tmp, wc, oh, ol);
        count = wc;
    }
#endif
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    if (count == 0 && bps == 4 && stride == 1)
    {
        const int order[4] =
#if WORDS_BIGENDIAN
            {0, 1, 2, 3};
#else
            {3, 2, 1, 0};
#endif
        interleave4_neon(cp, tmp, wc, order);
        count = wc;
    }
    else if (count == 0 && bps == 8 && stride == 1)
    {
        const int oh[4] =
#if WORDS_BIGENDIAN
            {0, 1, 2, 3};
#else
            {7, 6, 5, 4};
#endif
        const int ol[4] =
#if WORDS_BIGENDIAN
            {4, 5, 6, 7};
#else
            {3, 2, 1, 0};
#endif
        interleave8_neon(cp, tmp, wc, oh, ol);
        count = wc;
    }
#endif
    if (count == 0)
    {
        for (count = 0; count < wc; count++)
        {
            for (uint32_t byte = 0; byte < bps; byte++)
            {
#if WORDS_BIGENDIAN
                cp[byte * wc + count] = tmp[bps * count + byte];
#else
                cp[(bps - byte - 1) * wc + count] = tmp[bps * count + byte];
#endif
            }
        }
    }
    _TIFFfreeExt(tif, tmp);

    cp = (uint8_t *)cp0;
    cp += cc - stride - 1;
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    if (stride == 1 && cc > 1)
    {
        uint8_t *p = cp0 + 1;
        tmsize_t remaining = cc - 1;
        while (remaining >= 16)
        {
            __builtin_prefetch(p + 16);
            uint8x16_t cur = vld1q_u8(p);
            __builtin_prefetch(p + 64);
            uint8x16_t prev = vld1q_u8(p - 1);
            uint8x16_t diff = vsubq_u8(cur, prev);
            vst1q_u8(p, diff);
            p += 16;
            remaining -= 16;
        }
        while (remaining--)
        {
            *p = (uint8_t)(*p - *(p - 1));
            ++p;
        }
        return 1;
    }
#endif
    for (count = cc; count > stride; count -= stride)
        REPEAT4(stride,
                cp[stride] = (unsigned char)((cp[stride] - cp[0]) & 0xff);
                cp--)
    return 1;
}

static int PredictorEncodeRow(TIFF *tif, uint8_t *bp, tmsize_t cc, uint16_t s)
{
    static const char module[] = "PredictorEncodeRow";
    TIFFPredictorState *sp = PredictorState(tif);
    uint8_t *working_copy;
    int result_code;

    assert(sp != NULL);
    assert(sp->encodepfunc != NULL);
    assert(sp->encoderow != NULL);

    /*
     * Do predictor manipulation in a working buffer to avoid altering
     * the callers buffer, like for PredictorEncodeTile().
     * https://gitlab.com/libtiff/libtiff/-/issues/5
     */
    working_copy = (uint8_t *)_TIFFmallocExt(tif, cc);
    if (working_copy == NULL)
    {
        TIFFErrorExtR(tif, module,
                      "Out of memory allocating %" PRId64 " byte temp buffer.",
                      (int64_t)cc);
        return 0;
    }
    memcpy(working_copy, bp, cc);

    if (!(*sp->encodepfunc)(tif, working_copy, cc))
    {
        _TIFFfreeExt(tif, working_copy);
        return 0;
    }
    result_code = (*sp->encoderow)(tif, working_copy, cc, s);
    _TIFFfreeExt(tif, working_copy);
    return result_code;
}

static int PredictorEncodeTile(TIFF *tif, uint8_t *bp0, tmsize_t cc0,
                               uint16_t s)
{
    static const char module[] = "PredictorEncodeTile";
    TIFFPredictorState *sp = PredictorState(tif);
    uint8_t *working_copy;
    tmsize_t cc = cc0, rowsize;
    unsigned char *bp;
    int result_code;

    assert(sp != NULL);
    assert(sp->encodepfunc != NULL);
    assert(sp->encodetile != NULL);

    /*
     * Do predictor manipulation in a working buffer to avoid altering
     * the callers buffer. http://trac.osgeo.org/gdal/ticket/1965
     */
    working_copy = (uint8_t *)_TIFFmallocExt(tif, cc0);
    if (working_copy == NULL)
    {
        TIFFErrorExtR(tif, module,
                      "Out of memory allocating %" PRId64 " byte temp buffer.",
                      (int64_t)cc0);
        return 0;
    }
    memcpy(working_copy, bp0, cc0);
    bp = working_copy;

    rowsize = sp->rowsize;
    assert(rowsize > 0);
    if ((cc0 % rowsize) != 0)
    {
        TIFFErrorExtR(tif, "PredictorEncodeTile", "%s", "(cc0%rowsize)!=0");
        _TIFFfreeExt(tif, working_copy);
        return 0;
    }
    while (cc > 0)
    {
        (*sp->encodepfunc)(tif, bp, rowsize);
        cc -= rowsize;
        bp += rowsize;
    }
    result_code = (*sp->encodetile)(tif, working_copy, cc0, s);

    _TIFFfreeExt(tif, working_copy);

    return result_code;
}

#define FIELD_PREDICTOR (FIELD_CODEC + 0) /* XXX */

static const TIFFField predictFields[] = {
    {TIFFTAG_PREDICTOR, 1, 1, TIFF_SHORT, 0, TIFF_SETGET_UINT16,
     FIELD_PREDICTOR, FALSE, FALSE, "Predictor", NULL},
};

static int PredictorVSetField(TIFF *tif, uint32_t tag, va_list ap)
{
    TIFFPredictorState *sp = PredictorState(tif);

    assert(sp != NULL);
    assert(sp->vsetparent != NULL);

    switch (tag)
    {
        case TIFFTAG_PREDICTOR:
            sp->predictor = (uint16_t)va_arg(ap, uint16_vap);
            TIFFSetFieldBit(tif, FIELD_PREDICTOR);
            break;
        default:
            return (*sp->vsetparent)(tif, tag, ap);
    }
    tif->tif_flags |= TIFF_DIRTYDIRECT;
    return 1;
}

static int PredictorVGetField(TIFF *tif, uint32_t tag, va_list ap)
{
    TIFFPredictorState *sp = PredictorState(tif);

    assert(sp != NULL);
    assert(sp->vgetparent != NULL);

    switch (tag)
    {
        case TIFFTAG_PREDICTOR:
            *va_arg(ap, uint16_t *) = (uint16_t)sp->predictor;
            break;
        default:
            return (*sp->vgetparent)(tif, tag, ap);
    }
    return 1;
}

static void PredictorPrintDir(TIFF *tif, FILE *fd, long flags)
{
    TIFFPredictorState *sp = PredictorState(tif);

    (void)flags;
    if (TIFFFieldSet(tif, FIELD_PREDICTOR))
    {
        fprintf(fd, "  Predictor: ");
        switch (sp->predictor)
        {
            case 1:
                fprintf(fd, "none ");
                break;
            case 2:
                fprintf(fd, "horizontal differencing ");
                break;
            case 3:
                fprintf(fd, "floating point predictor ");
                break;
        }
        fprintf(fd, "%d (0x%x)\n", sp->predictor, sp->predictor);
    }
    if (sp->printdir)
        (*sp->printdir)(tif, fd, flags);
}

int TIFFPredictorInit(TIFF *tif)
{
    TIFFPredictorState *sp = PredictorState(tif);

    assert(sp != 0);

    /*
     * Merge codec-specific tag information.
     */
    if (!_TIFFMergeFields(tif, predictFields, TIFFArrayCount(predictFields)))
    {
        TIFFErrorExtR(tif, "TIFFPredictorInit",
                      "Merging Predictor codec-specific tags failed");
        return 0;
    }

    /*
     * Override parent get/set field methods.
     */
    sp->vgetparent = tif->tif_tagmethods.vgetfield;
    tif->tif_tagmethods.vgetfield =
        PredictorVGetField; /* hook for predictor tag */
    sp->vsetparent = tif->tif_tagmethods.vsetfield;
    tif->tif_tagmethods.vsetfield =
        PredictorVSetField; /* hook for predictor tag */
    sp->printdir = tif->tif_tagmethods.printdir;
    tif->tif_tagmethods.printdir =
        PredictorPrintDir; /* hook for predictor tag */

    sp->setupdecode = tif->tif_setupdecode;
    tif->tif_setupdecode = PredictorSetupDecode;
    sp->setupencode = tif->tif_setupencode;
    tif->tif_setupencode = PredictorSetupEncode;

    sp->predictor = 1;      /* default value */
    sp->encodepfunc = NULL; /* no predictor routine */
    sp->decodepfunc = NULL; /* no predictor routine */
    return 1;
}

int TIFFPredictorCleanup(TIFF *tif)
{
    TIFFPredictorState *sp = PredictorState(tif);

    assert(sp != 0);

    tif->tif_tagmethods.vgetfield = sp->vgetparent;
    tif->tif_tagmethods.vsetfield = sp->vsetparent;
    tif->tif_tagmethods.printdir = sp->printdir;
    tif->tif_setupdecode = sp->setupdecode;
    tif->tif_setupencode = sp->setupencode;

    return 1;
}
