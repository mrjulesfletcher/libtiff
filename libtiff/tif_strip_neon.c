#include "tif_bayer.h"
#include "tiffiop.h"
#include <string.h>
#if defined(HAVE_NEON) && defined(__ARM_NEON)
#include <arm_neon.h>
#endif

static void horiz_diff16_neon(uint16_t *row, uint32_t width)
{
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    if (width <= 1)
        return;
    uint16_t *p = row + 1;
    uint32_t remaining = width - 1;
    while (remaining >= 16)
    {
        uint16x8_t cur0 = vld1q_u16(p);
        uint16x8_t cur1 = vld1q_u16(p + 8);
        uint16x8_t prev0 = vld1q_u16(p - 1);
        uint16x8_t prev1 = vld1q_u16(p + 7);
        vst1q_u16(p, vsubq_u16(cur0, prev0));
        vst1q_u16(p + 8, vsubq_u16(cur1, prev1));
        p += 16;
        remaining -= 16;
    }
    if (remaining >= 8)
    {
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
#else
    if (width <= 1)
        return;
    for (uint32_t i = 1; i < width; i++)
        row[i] = (uint16_t)(row[i] - row[i - 1]);
#endif
}

uint8_t *TIFFAssembleStripNEON(TIFF *tif, const uint16_t *src, uint32_t width,
                               uint32_t height, int apply_predictor,
                               int bigendian, size_t *out_size)
{
    static const char module[] = "TIFFAssembleStripNEON";
    uint64_t count64 = _TIFFMultiply64(tif, width, height, module);
    if (count64 == 0 && width != 0 && height != 0)
        return NULL;
    tmsize_t countm = _TIFFCastUInt64ToSSize(tif, count64, module);
    if (countm == 0 && count64 != 0)
        return NULL;
    size_t count = (size_t)countm;
    uint16_t *tmp = (uint16_t *)_TIFFmallocExt(tif, count * sizeof(uint16_t));
    if (!tmp)
    {
        TIFFErrorExtR(tif, module, "Out of memory");
        return NULL;
    }
    memcpy(tmp, src, count * sizeof(uint16_t));
    if (apply_predictor)
    {
        for (uint32_t row = 0; row < height; row++)
            horiz_diff16_neon(tmp + row * width, width);
    }
    size_t packed_size = ((count + 1) / 2) * 3;
    uint8_t *packed = (uint8_t *)_TIFFmallocExt(tif, packed_size);
    if (!packed)
    {
        _TIFFfreeExt(tif, tmp);
        TIFFErrorExtR(tif, module, "Out of memory");
        return NULL;
    }
    TIFFPackRaw12(tmp, packed, count, bigendian);
    _TIFFfreeExt(tif, tmp);
    if (out_size)
        *out_size = packed_size;
    return packed;
}
