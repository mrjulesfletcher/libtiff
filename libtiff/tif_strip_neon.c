#include "tif_bayer.h"
#include "tiff_simd.h"
#include "tiffiop.h"
#include <stdbool.h>
#include <string.h>
#if defined(HAVE_NEON) && defined(__ARM_NEON)
#include <arm_neon.h>
#endif

static void horiz_diff16(uint16_t *row, uint32_t width)
{
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    if (tiff_use_neon)
    {
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
        return;
    }
#endif
    if (width <= 1)
        return;
    for (uint32_t i = 1; i < width; i++)
        row[i] = (uint16_t)(row[i] - row[i - 1]);
}

uint8_t *TIFFAssembleStripNEON(TIFF *tif, const uint16_t *src, uint32_t width,
                               uint32_t height, int apply_predictor,
                               int bigendian, size_t *out_size,
                               uint16_t *scratch, uint8_t *out_buf)
{
    static const char module[] = "TIFFAssembleStripNEON";
    uint64_t count64 = _TIFFMultiply64(tif, width, height, module);
    if (count64 == 0 && width != 0 && height != 0)
        return NULL;
    tmsize_t countm = _TIFFCastUInt64ToSSize(tif, count64, module);
    if (countm == 0 && count64 != 0)
        return NULL;
    size_t count = (size_t)countm;

    bool free_scratch = false;
    if (apply_predictor && scratch == NULL)
    {
        scratch = (uint16_t *)_TIFFmallocExt(tif, count * sizeof(uint16_t));
        if (!scratch)
        {
            TIFFErrorExtR(tif, module, "Out of memory");
            return NULL;
        }
        free_scratch = true;
    }
    if (apply_predictor)
    {
        memcpy(scratch, src, count * sizeof(uint16_t));
        for (uint32_t row = 0; row < height; row++)
            horiz_diff16(scratch + row * width, width);
    }

    size_t packed_size = ((count + 1) / 2) * 3;
    if (out_buf == NULL)
    {
        out_buf = (uint8_t *)_TIFFmallocExt(tif, packed_size);
        if (!out_buf)
        {
            if (free_scratch)
                _TIFFfreeExt(tif, scratch);
            TIFFErrorExtR(tif, module, "Out of memory");
            return NULL;
        }
    }

    const uint16_t *pack_src = apply_predictor ? scratch : src;
    TIFFPackRaw12(pack_src, out_buf, count, bigendian);

    if (free_scratch)
        _TIFFfreeExt(tif, scratch);

    if (out_size)
        *out_size = packed_size;
    return out_buf;
}
