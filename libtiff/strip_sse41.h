#ifndef STRIP_SSE41_H
#define STRIP_SSE41_H

#include "tiffio.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    uint8_t *TIFFAssembleStripSSE41(TIFF *tif, const uint16_t *src,
                                    uint32_t width, uint32_t height,
                                    int apply_predictor, int bigendian,
                                    size_t *out_size, uint16_t *scratch,
                                    uint8_t *out_buf);

#ifdef __cplusplus
}
#endif

#endif /* STRIP_SSE41_H */
