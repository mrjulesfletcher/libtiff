#include "tif_bayer.h"
#include "tiff_simd.h"
#include "tiffiop.h"
#include <stdbool.h>
#include <string.h>
#if defined(HAVE_SSE41)
#include <smmintrin.h>
#endif

static void horiz_diff16(uint16_t *row, uint32_t width)
{
#if defined(HAVE_SSE41)
    if (tiff_use_sse41)
    {
        if (width <= 1)
            return;
        uint16_t *p = row + 1;
        uint32_t remaining = width - 1;
        __m128i prev = _mm_set1_epi16((short)row[0]);
        while (remaining >= 8)
        {
            __m128i cur = _mm_loadu_si128((const __m128i *)p);
            __m128i prev_vec = _mm_alignr_epi8(cur, prev, 2);
            __m128i diff = _mm_sub_epi16(cur, prev_vec);
            _mm_storeu_si128((__m128i *)p, diff);
            prev = cur;
            p += 8;
            remaining -= 8;
        }
        uint16_t acc = (uint16_t)_mm_extract_epi16(prev, 7);
        while (remaining--)
        {
            uint16_t curWord = *p;
            *p = (uint16_t)(curWord - acc);
            acc = curWord;
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

typedef struct
{
    TIFF *tif;
    const uint16_t *src;
    uint32_t width;
    uint32_t height;
    int apply_predictor;
    int bigendian;
    size_t *out_size;
    uint16_t *scratch;
    uint8_t *out_buf;
    uint8_t *result;
} TPStripTask;

static uint8_t *assemble_strip_sse41_internal(TIFF *tif, const uint16_t *src,
                                              uint32_t width, uint32_t height,
                                              int apply_predictor,
                                              int bigendian, size_t *out_size,
                                              uint16_t *scratch,
                                              uint8_t *out_buf);

static void assemble_strip_sse41_task(void *arg)
{
    TPStripTask *t = (TPStripTask *)arg;
    t->result = assemble_strip_sse41_internal(
        t->tif, t->src, t->width, t->height, t->apply_predictor, t->bigendian,
        t->out_size, t->scratch, t->out_buf);
}

uint8_t *TIFFAssembleStripSSE41(TIFF *tif, const uint16_t *src, uint32_t width,
                                uint32_t height, int apply_predictor,
                                int bigendian, size_t *out_size,
                                uint16_t *scratch, uint8_t *out_buf)
{
    static const char module[] = "TIFFAssembleStripSSE41";
    TPStripTask task = {tif,       src,      width,   height,  apply_predictor,
                        bigendian, out_size, scratch, out_buf, NULL};
#ifdef TIFF_USE_THREADPOOL
    if (tif && TIFFGetThreadCount(tif) > 1)
    {
        _TIFFThreadPoolSubmit(tif->tif_threadpool, assemble_strip_sse41_task,
                              &task);
        _TIFFThreadPoolWait(tif->tif_threadpool);
        return task.result;
    }
#endif
    return assemble_strip_sse41_internal(tif, src, width, height,
                                         apply_predictor, bigendian, out_size,
                                         scratch, out_buf);
}

static uint8_t *assemble_strip_sse41_internal(TIFF *tif, const uint16_t *src,
                                              uint32_t width, uint32_t height,
                                              int apply_predictor,
                                              int bigendian, size_t *out_size,
                                              uint16_t *scratch,
                                              uint8_t *out_buf)
{
    static const char module[] = "TIFFAssembleStripSSE41";
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
