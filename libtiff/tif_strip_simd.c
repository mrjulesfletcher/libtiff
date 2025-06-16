#include "strip_neon.h"
#include "strip_sse41.h"
#include "tiff_simd.h"

uint8_t *TIFFAssembleStripSIMD(TIFF *tif, const uint16_t *src, uint32_t width,
                               uint32_t height, int apply_predictor,
                               int bigendian, size_t *out_size,
                               uint16_t *scratch, uint8_t *out_buf)
{
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    if (TIFFUseNEON())
        return TIFFAssembleStripNEON(tif, src, width, height, apply_predictor,
                                     bigendian, out_size, scratch, out_buf);
#endif
#if defined(HAVE_SSE41)
    if (TIFFUseSSE41())
        return TIFFAssembleStripSSE41(tif, src, width, height, apply_predictor,
                                      bigendian, out_size, scratch, out_buf);
#endif
#if defined(HAVE_NEON) && defined(__ARM_NEON)
    return TIFFAssembleStripNEON(tif, src, width, height, apply_predictor,
                                 bigendian, out_size, scratch, out_buf);
#elif defined(HAVE_SSE41)
    return TIFFAssembleStripSSE41(tif, src, width, height, apply_predictor,
                                  bigendian, out_size, scratch, out_buf);
#else
    /* Scalar path compiled in NEON implementation */
    return TIFFAssembleStripNEON(tif, src, width, height, apply_predictor,
                                 bigendian, out_size, scratch, out_buf);
#endif
}
