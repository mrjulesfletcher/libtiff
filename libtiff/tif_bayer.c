#include "tif_bayer.h"
#include <string.h>

#if defined(HAVE_NEON) && defined(__ARM_NEON)
#include <arm_neon.h>
#endif

static void pack12_scalar(const uint16_t *src, uint8_t *dst, size_t count,
                          int bigendian)
{
    for (size_t i = 0; i < count; i += 2)
    {
        uint16_t a = src[i];
        uint16_t b = src[i + 1];
        if (!bigendian)
        {
            dst[0] = (uint8_t)(a & 0xff);
            dst[1] = (uint8_t)(((a >> 8) & 0x0f) | ((b & 0x0f) << 4));
            dst[2] = (uint8_t)((b >> 4) & 0xff);
        }
        else
        {
            dst[0] = (uint8_t)((a >> 4) & 0xff);
            dst[1] = (uint8_t)(((a & 0x0f) << 4) | ((b >> 8) & 0x0f));
            dst[2] = (uint8_t)(b & 0xff);
        }
        dst += 3;
    }
}

static void unpack12_scalar(const uint8_t *src, uint16_t *dst, size_t count,
                            int bigendian)
{
    for (size_t i = 0; i < count; i += 2)
    {
        if (!bigendian)
        {
            dst[i] = (uint16_t)(src[0] | ((src[1] & 0x0f) << 8));
            dst[i + 1] = (uint16_t)((src[1] >> 4) | (src[2] << 4));
        }
        else
        {
            dst[i] = (uint16_t)((src[0] << 4) | (src[1] >> 4));
            dst[i + 1] = (uint16_t)(((src[1] & 0x0f) << 8) | src[2]);
        }
        src += 3;
    }
}

#if defined(HAVE_NEON) && defined(__ARM_NEON)
static void pack12_neon(const uint16_t *src, uint8_t *dst, size_t count,
                        int bigendian)
{
    size_t i = 0;
    uint8x8_t mask4 = vdup_n_u8(0x0f);
    for (; i + 16 <= count; i += 16)
    {
        __builtin_prefetch(src + i + 32);
        uint16x8x2_t v = vld2q_u16(src + i);
        uint16x8_t even = v.val[0];
        uint16x8_t odd = v.val[1];

        uint8x8_t even_lo = vmovn_u16(even);
        uint8x8_t even_mid = vshrn_n_u16(even, 4);
        uint8x8_t even_hi = vshrn_n_u16(even, 8);
        uint8x8_t odd_lo = vmovn_u16(odd);
        uint8x8_t odd_mid = vshrn_n_u16(odd, 4);
        uint8x8_t odd_hi = vshrn_n_u16(odd, 8);

        uint8x8x3_t outv;
        if (!bigendian)
        {
            outv.val[0] = even_lo;
            outv.val[1] = vorr_u8(vand_u8(even_hi, mask4),
                                  vshl_n_u8(vand_u8(odd_lo, mask4), 4));
            outv.val[2] = odd_mid;
        }
        else
        {
            outv.val[0] = even_mid;
            outv.val[1] = vorr_u8(vshl_n_u8(vand_u8(even_lo, mask4), 4),
                                  vand_u8(odd_hi, mask4));
            outv.val[2] = odd_lo;
        }
        vst3_u8(dst, outv);
        dst += 24;
    }
    if (i < count)
        pack12_scalar(src + i, dst, count - i, bigendian);
}

static void unpack12_neon(const uint8_t *src, uint16_t *dst, size_t count,
                          int bigendian)
{
    size_t i = 0;
    uint8x8_t mask4 = vdup_n_u8(0x0f);
    for (; i + 16 <= count; i += 16)
    {
        __builtin_prefetch(src + 32);
        uint8x8x3_t v = vld3_u8(src);
        uint16x8_t out0, out1;
        if (!bigendian)
        {
            out0 = vorrq_u16(vmovl_u8(v.val[0]),
                             vshll_n_u8(vand_u8(v.val[1], mask4), 8));
            out1 = vorrq_u16(vshll_n_u8(vshr_n_u8(v.val[1], 4), 4),
                             vshll_n_u8(v.val[2], 4));
        }
        else
        {
            out0 = vorrq_u16(vshll_n_u8(v.val[0], 4),
                             vshll_n_u8(vshr_n_u8(v.val[1], 4), 0));
            out1 = vorrq_u16(vshll_n_u8(vand_u8(v.val[1], mask4), 8),
                             vmovl_u8(v.val[2]));
        }
        uint16x8x2_t res = {out0, out1};
        vst2q_u16(dst + i, res);
        src += 24;
    }
    if (i < count)
        unpack12_scalar(src, dst + i, count - i, bigendian);
}
#endif /* HAVE_NEON */

void TIFFPackRaw12(const uint16_t *src, uint8_t *dst, size_t count, int bigendian)
{
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    pack12_neon(src, dst, count, bigendian);
#else
    pack12_scalar(src, dst, count, bigendian);
#endif
}

void TIFFUnpackRaw12(const uint8_t *src, uint16_t *dst, size_t count,
                     int bigendian)
{
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    unpack12_neon(src, dst, count, bigendian);
#else
    unpack12_scalar(src, dst, count, bigendian);
#endif
}

